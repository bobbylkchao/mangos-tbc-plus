/* ScriptData
SDName: npc_craftsman_board
SD%Complete: 100
SDComment: City craftsman board offering paid PlayerBot enchanting for equipped gear
SDCategory: Custom
EndScriptData */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "Entities/Item.h"
#include "Server/DBCStores.h"
#include "Spells/SpellMgr.h"
#include "World/World.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/RandomPlayerbotMgr.h"
#endif

namespace
{
    uint32 const CRAFTSMAN_ENCHANT_FEE = 200 * GOLD;
    uint32 const CRAFTSMAN_PAGE_SIZE = 8;

    enum CraftsmanActions
    {
        ACTION_MAIN             = GOSSIP_ACTION_INFO_DEF,
        ACTION_SLOT_FIRST       = GOSSIP_ACTION_INFO_DEF + 10,
        ACTION_SLOT_LAST        = ACTION_SLOT_FIRST + EQUIPMENT_SLOT_END,
        ACTION_PAGE_FIRST       = GOSSIP_ACTION_INFO_DEF + 100,
        ACTION_PAGE_LAST        = ACTION_PAGE_FIRST + 50,
        ACTION_ENCHANT_FIRST    = GOSSIP_ACTION_INFO_DEF + 200,
        ACTION_ENCHANT_LAST     = ACTION_ENCHANT_FIRST + CRAFTSMAN_PAGE_SIZE,
        ACTION_CONFIRM          = GOSSIP_ACTION_INFO_DEF + 300,
        ACTION_BACK_SLOTS       = GOSSIP_ACTION_INFO_DEF + 301,
        ACTION_BACK_ENCHANTS    = GOSSIP_ACTION_INFO_DEF + 302
    };

    struct CraftsmanSession
    {
        uint8 equipSlot = EQUIPMENT_SLOT_END;
        uint32 page = 0;
        uint32 selectedSpellId = 0;
        std::vector<uint32> spellIds;
    };

    std::map<ObjectGuid, CraftsmanSession> s_craftsmanSessions;

    char const* GetSlotLabel(uint8 slot)
    {
        switch (slot)
        {
            case EQUIPMENT_SLOT_HEAD:      return "Head";
            case EQUIPMENT_SLOT_SHOULDERS: return "Shoulders";
            case EQUIPMENT_SLOT_CHEST:     return "Chest";
            case EQUIPMENT_SLOT_WAIST:     return "Waist";
            case EQUIPMENT_SLOT_LEGS:      return "Legs";
            case EQUIPMENT_SLOT_FEET:      return "Feet";
            case EQUIPMENT_SLOT_WRISTS:    return "Wrists";
            case EQUIPMENT_SLOT_HANDS:     return "Hands";
            case EQUIPMENT_SLOT_FINGER1:   return "Finger 1";
            case EQUIPMENT_SLOT_FINGER2:   return "Finger 2";
            case EQUIPMENT_SLOT_BACK:      return "Cloak";
            case EQUIPMENT_SLOT_MAINHAND:  return "Main Hand";
            case EQUIPMENT_SLOT_OFFHAND:   return "Off Hand";
            case EQUIPMENT_SLOT_RANGED:    return "Ranged";
            default:                       return nullptr;
        }
    }

    bool IsEnchantableSlot(uint8 slot)
    {
        return GetSlotLabel(slot) != nullptr;
    }

    CraftsmanSession& GetSession(Player* player)
    {
        return s_craftsmanSessions[player->GetObjectGuid()];
    }

    void ClearSession(Player* player)
    {
        s_craftsmanSessions.erase(player->GetObjectGuid());
    }

    uint32 GetEnchantIdFromSpell(SpellEntry const* spellInfo)
    {
        if (!spellInfo)
            return 0;

        for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (spellInfo->Effect[i] == SPELL_EFFECT_ENCHANT_ITEM)
                return spellInfo->EffectMiscValue[i];
        }
        return 0;
    }

    std::string GetEnchantLabel(SpellEntry const* spellInfo, LocaleConstant locale)
    {
        uint32 enchantId = GetEnchantIdFromSpell(spellInfo);
        if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchantId))
        {
            if (enchantEntry->description[locale] && *enchantEntry->description[locale])
                return enchantEntry->description[locale];
            if (enchantEntry->description[0] && *enchantEntry->description[0])
                return enchantEntry->description[0];
        }

        if (spellInfo->SpellName[locale] && *spellInfo->SpellName[locale])
            return spellInfo->SpellName[locale];
        if (spellInfo->SpellName[0] && *spellInfo->SpellName[0])
            return spellInfo->SpellName[0];

        return "Unknown Enchant";
    }

    void CollectEnchantsForItem(Item* item, std::vector<uint32>& outSpellIds)
    {
        outSpellIds.clear();
        if (!item)
            return;

        std::map<uint32, uint32> bestSpellByEnchant; // enchantId -> spellId

        SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBoundsBySkillId(SKILL_ENCHANTING);
        for (SkillLineAbilityMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        {
            SkillLineAbilityEntry const* ability = itr->second;
            if (!ability)
                continue;

            SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(ability->spellId);
            if (!spellInfo)
                continue;

            uint32 enchantId = GetEnchantIdFromSpell(spellInfo);
            if (!enchantId || !sSpellItemEnchantmentStore.LookupEntry(enchantId))
                continue;

            // Permanent enchants only; oils / whetstones use a different effect.
            bool hasTemp = false;
            for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                if (spellInfo->Effect[i] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)
                    hasTemp = true;
            }
            if (hasTemp)
                continue;

            if (!item->IsFitToSpellRequirements(spellInfo))
                continue;

            auto existing = bestSpellByEnchant.find(enchantId);
            if (existing == bestSpellByEnchant.end())
            {
                bestSpellByEnchant[enchantId] = ability->spellId;
                continue;
            }

            // Keep the spell that requires the higher skill rank when duplicates exist.
            SkillLineAbilityMapBounds oldBounds = sSpellMgr.GetSkillLineAbilityMapBoundsBySpellId(existing->second);
            uint32 oldReq = 0;
            if (oldBounds.first != oldBounds.second)
                oldReq = oldBounds.first->second->req_skill_value;

            if (ability->req_skill_value >= oldReq)
                bestSpellByEnchant[enchantId] = ability->spellId;
        }

        outSpellIds.reserve(bestSpellByEnchant.size());
        for (auto const& pair : bestSpellByEnchant)
            outSpellIds.push_back(pair.second);

        LocaleConstant locale = sWorld.GetDefaultDbcLocale();
        std::sort(outSpellIds.begin(), outSpellIds.end(), [locale](uint32 left, uint32 right)
        {
            SpellEntry const* leftSpell = sSpellTemplate.LookupEntry<SpellEntry>(left);
            SpellEntry const* rightSpell = sSpellTemplate.LookupEntry<SpellEntry>(right);
            return GetEnchantLabel(leftSpell, locale) < GetEnchantLabel(rightSpell, locale);
        });
    }

    Player* FindServiceBot()
    {
#ifdef ENABLE_PLAYERBOTS
        Player* found = nullptr;
        sRandomPlayerbotMgr.ForEachPlayerbot([&found](Player* bot)
        {
            if (found || !bot || !bot->IsInWorld() || !bot->IsAlive())
                return;
            found = bot;
        });
        return found;
#else
        return nullptr;
#endif
    }

    void SendBoardMenu(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "Request an enchant (200 gold)", GOSSIP_SENDER_MAIN, ACTION_MAIN);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendSlotMenu(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Choose an equipped item to enchant:", GOSSIP_SENDER_MAIN, ACTION_BACK_SLOTS);

        uint32 options = 0;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (!IsEnchantableSlot(slot))
                continue;

            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;

            char label[128];
            snprintf(label, sizeof(label), "%s: [%s]", GetSlotLabel(slot), item->GetProto()->Name1);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, label, GOSSIP_SENDER_MAIN, ACTION_SLOT_FIRST + slot);
            ++options;
        }

        if (!options)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "No equipped enchantable items found.", GOSSIP_SENDER_MAIN, ACTION_BACK_SLOTS);

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_MAIN);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendEnchantMenu(Player* player, Creature* creature)
    {
        CraftsmanSession& session = GetSession(player);
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, session.equipSlot);
        if (!item)
        {
            player->GetSession()->SendAreaTriggerMessage("That item is no longer equipped.");
            ClearSession(player);
            SendSlotMenu(player, creature);
            return;
        }

        if (session.spellIds.empty())
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "No permanent enchants are available for this item.", GOSSIP_SENDER_MAIN, ACTION_BACK_SLOTS);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK_SLOTS);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
            return;
        }

        uint32 pageCount = (session.spellIds.size() + CRAFTSMAN_PAGE_SIZE - 1) / CRAFTSMAN_PAGE_SIZE;
        if (session.page >= pageCount)
            session.page = pageCount - 1;

        char header[160];
        snprintf(header, sizeof(header), "Enchants for [%s] (page %u/%u):",
            item->GetProto()->Name1, session.page + 1, pageCount);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, header, GOSSIP_SENDER_MAIN, ACTION_BACK_ENCHANTS);

        LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
        uint32 start = session.page * CRAFTSMAN_PAGE_SIZE;
        uint32 end = std::min<uint32>(start + CRAFTSMAN_PAGE_SIZE, uint32(session.spellIds.size()));
        for (uint32 i = start; i < end; ++i)
        {
            SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(session.spellIds[i]);
            std::string label = GetEnchantLabel(spellInfo, locale) + " - 200 gold";
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, label.c_str(), GOSSIP_SENDER_MAIN,
                ACTION_ENCHANT_FIRST + (i - start));
        }

        if (session.page > 0)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Previous page", GOSSIP_SENDER_MAIN, ACTION_PAGE_FIRST + (session.page - 1));
        if (session.page + 1 < pageCount)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Next page", GOSSIP_SENDER_MAIN, ACTION_PAGE_FIRST + (session.page + 1));

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, ACTION_BACK_SLOTS);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    void SendConfirmMenu(Player* player, Creature* creature)
    {
        CraftsmanSession& session = GetSession(player);
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, session.equipSlot);
        SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(session.selectedSpellId);
        if (!item || !spellInfo)
        {
            player->GetSession()->SendAreaTriggerMessage("Enchant request expired. Please try again.");
            ClearSession(player);
            SendSlotMenu(player, creature);
            return;
        }

        LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
        std::string enchantName = GetEnchantLabel(spellInfo, locale);

        char summary[256];
        snprintf(summary, sizeof(summary), "Enchant [%s] with %s for 200 gold?",
            item->GetProto()->Name1, enchantName.c_str());
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, summary, GOSSIP_SENDER_MAIN, ACTION_BACK_ENCHANTS);

        char boxMessage[160];
        snprintf(boxMessage, sizeof(boxMessage), "Pay 200 gold to enchant [%s]?", item->GetProto()->Name1);
        player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_MONEY_BAG, "I accept", GOSSIP_SENDER_MAIN, ACTION_CONFIRM,
            boxMessage, CRAFTSMAN_ENCHANT_FEE, false);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Cancel", GOSSIP_SENDER_MAIN, ACTION_BACK_ENCHANTS);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    }

    bool ApplyPaidEnchant(Player* player, Creature* creature)
    {
        CraftsmanSession& session = GetSession(player);
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, session.equipSlot);
        SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(session.selectedSpellId);
        uint32 enchantId = GetEnchantIdFromSpell(spellInfo);

        if (!item || !spellInfo || !enchantId || !item->IsFitToSpellRequirements(spellInfo))
        {
            player->GetSession()->SendAreaTriggerMessage("Unable to apply that enchant to the selected item.");
            return false;
        }

#ifndef ENABLE_PLAYERBOTS
        player->GetSession()->SendAreaTriggerMessage("Craftsman Board requires PlayerBots to be enabled.");
        return false;
#else
        Player* bot = FindServiceBot();
        if (!bot)
        {
            player->GetSession()->SendAreaTriggerMessage("No enchanter bots are available right now. Please try again later.");
            return false;
        }

        if (player->GetMoney() < CRAFTSMAN_ENCHANT_FEE)
        {
            player->GetSession()->SendAreaTriggerMessage("You need 200 gold for this enchant.");
            return false;
        }

        player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
        item->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchantId, 0, 0, bot->GetObjectGuid());
        player->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);

        player->ModifyMoney(-int32(CRAFTSMAN_ENCHANT_FEE));
        bot->ModifyMoney(int32(CRAFTSMAN_ENCHANT_FEE));

        LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
        std::string enchantName = GetEnchantLabel(spellInfo, locale);

        char message[256];
        snprintf(message, sizeof(message), "%s enchanted your [%s] with %s. -200 gold",
            bot->GetName(), item->GetProto()->Name1, enchantName.c_str());
        player->GetSession()->SendAreaTriggerMessage("%s", message);
        creature->MonsterWhisper(message, player);

        return true;
#endif
    }
} // namespace

bool GossipHello_npc_craftsman_board(Player* player, Creature* creature)
{
    ClearSession(player);

    if (!sWorld.getConfig(CONFIG_BOOL_CRAFTSMAN_BOARD_ENABLED))
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "The Craftsman Board is currently closed.", GOSSIP_SENDER_MAIN, ACTION_MAIN);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
        return true;
    }

#ifndef ENABLE_PLAYERBOTS
    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Enchanting services require a PlayerBots build.", GOSSIP_SENDER_MAIN, ACTION_MAIN);
    player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
    return true;
#else
    SendBoardMenu(player, creature);
    return true;
#endif
}

bool GossipSelect_npc_craftsman_board(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
    if (!sWorld.getConfig(CONFIG_BOOL_CRAFTSMAN_BOARD_ENABLED))
    {
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    if (action == ACTION_MAIN)
    {
        ClearSession(player);
        SendSlotMenu(player, creature);
        return true;
    }

    if (action == ACTION_BACK_SLOTS)
    {
        CraftsmanSession& session = GetSession(player);
        session.selectedSpellId = 0;
        session.spellIds.clear();
        session.page = 0;
        SendSlotMenu(player, creature);
        return true;
    }

    if (action == ACTION_BACK_ENCHANTS)
    {
        GetSession(player).selectedSpellId = 0;
        SendEnchantMenu(player, creature);
        return true;
    }

    if (action >= ACTION_SLOT_FIRST && action < ACTION_SLOT_LAST)
    {
        uint8 slot = uint8(action - ACTION_SLOT_FIRST);
        if (!IsEnchantableSlot(slot) || !player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            player->GetSession()->SendAreaTriggerMessage("That equipment slot is empty.");
            SendSlotMenu(player, creature);
            return true;
        }

        CraftsmanSession& session = GetSession(player);
        session.equipSlot = slot;
        session.page = 0;
        session.selectedSpellId = 0;
        CollectEnchantsForItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot), session.spellIds);
        SendEnchantMenu(player, creature);
        return true;
    }

    if (action >= ACTION_PAGE_FIRST && action <= ACTION_PAGE_LAST)
    {
        GetSession(player).page = action - ACTION_PAGE_FIRST;
        SendEnchantMenu(player, creature);
        return true;
    }

    if (action >= ACTION_ENCHANT_FIRST && action < ACTION_ENCHANT_LAST)
    {
        CraftsmanSession& session = GetSession(player);
        uint32 index = session.page * CRAFTSMAN_PAGE_SIZE + (action - ACTION_ENCHANT_FIRST);
        if (index >= session.spellIds.size())
        {
            SendEnchantMenu(player, creature);
            return true;
        }

        session.selectedSpellId = session.spellIds[index];
        SendConfirmMenu(player, creature);
        return true;
    }

    if (action == ACTION_CONFIRM)
    {
        if (ApplyPaidEnchant(player, creature))
        {
            ClearSession(player);
            player->CLOSE_GOSSIP_MENU();
        }
        else
            SendConfirmMenu(player, creature);
        return true;
    }

    player->CLOSE_GOSSIP_MENU();
    return true;
}

void AddSC_npc_craftsman_board()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "npc_craftsman_board";
    pNewScript->pGossipHello = &GossipHello_npc_craftsman_board;
    pNewScript->pGossipSelect = &GossipSelect_npc_craftsman_board;
    pNewScript->RegisterSelf();
}
