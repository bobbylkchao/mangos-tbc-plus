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
(910001, 910001, 0,   1, -8850.47,   654.58,   96.70, 0.60, 300, 300, 0, 0), -- Stormwind
(910002, 910001, 0,   1, -4950.00, -1190.00,  501.50, 2.20, 300, 300, 0, 0), -- Ironforge
(910003, 910001, 1,   1, 10106.00,  2575.00, 1322.00, 5.50, 300, 300, 0, 0), -- Darnassus
(910004, 910001, 530, 1, -3870.00,-11645.00, -137.60, 2.20, 300, 300, 0, 0), -- Exodar
(910005, 910001, 1,   1,  1678.00, -4430.00,   19.50, 1.80, 300, 300, 0, 0), -- Orgrimmar
(910006, 910001, 1,   1, -1209.00,    50.00,  140.00, 3.10, 300, 300, 0, 0), -- Thunder Bluff
(910007, 910001, 0,   1,  1633.00,   220.00,  -43.00, 2.50, 300, 300, 0, 0), -- Undercity
(910008, 910001, 530, 1,  9680.00, -7295.00,   14.50, 4.00, 300, 300, 0, 0), -- Silvermoon
(910009, 910001, 530, 1, -1828.00,  5426.00,  -12.40, 1.00, 300, 300, 0, 0); -- Shattrath
