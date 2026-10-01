ALTER TABLE db_version CHANGE COLUMN required_s2493_01_mangos_plainsrunning required_s2494_01_mangos_craftsman_board bit;

-- Craftsman Board NPC: paid PlayerBot enchanting for equipped gear in major cities.
DELETE FROM `creature` WHERE `id` = 910001;
DELETE FROM `creature_template` WHERE `Entry` = 910001;

INSERT INTO `creature_template`
(`Entry`, `Name`, `SubName`, `MinLevel`, `MaxLevel`, `DisplayId1`, `Faction`, `NpcFlags`, `UnitFlags`, `ExtraFlags`,
 `SpeedWalk`, `SpeedRun`, `Scale`, `CreatureType`, `InhabitType`, `RegenerateStats`, `UnitClass`, `Rank`,
 `HealthMultiplier`, `PowerMultiplier`, `DamageMultiplier`, `ArmorMultiplier`,
 `MinLevelHealth`, `MaxLevelHealth`, `MinLevelMana`, `MaxLevelMana`, `Armor`, `ScriptName`)
VALUES
(910001, 'Craftsman Board', 'Enchanting Services', 70, 70, 1298, 35, 1, 768, 2,
 1, 1.14286, 1, 7, 3, 3, 1, 0,
 1, 1, 1, 1,
 5000, 5000, 0, 0, 0, 'npc_craftsman_board');

-- One board per major city + Shattrath.
INSERT INTO `creature`
(`guid`, `id`, `map`, `spawnMask`, `position_x`, `position_y`, `position_z`, `orientation`,
 `spawntimesecsmin`, `spawntimesecsmax`, `spawndist`, `MovementType`)
VALUES
(910001, 910001, 0,   1, -8812.00,   666.35,   96.90, 1.73, 300, 300, 0, 0), -- Stormwind
(910002, 910001, 0,   1, -4956.43,  -909.20,  503.92, 0.70, 300, 300, 0, 0), -- Ironforge
(910003, 910001, 1,   1,  9862.00,  2339.19, 1321.67, 0.80, 300, 300, 0, 0), -- Darnassus
(910004, 910001, 530, 1, -4020.00,-11733.50, -151.81, 0.50, 300, 300, 0, 0), -- Exodar
(910005, 910001, 1,   1,  1679.00, -4450.11,   20.15, 1.90, 300, 300, 0, 0), -- Orgrimmar
(910006, 910001, 1,   1, -1198.00,   102.05,  134.80, 3.05, 300, 300, 0, 0), -- Thunder Bluff
(910007, 910001, 0,   1,  1650.00,   240.19,  -56.79, 0.10, 300, 300, 0, 0), -- Undercity
(910008, 910001, 530, 1,  9683.17, -7518.94,   18.26, 1.62, 300, 300, 0, 0), -- Silvermoon
(910009, 910001, 530, 1, -1853.68,  5430.04,   -9.71, 3.14, 300, 300, 0, 0); -- Shattrath
