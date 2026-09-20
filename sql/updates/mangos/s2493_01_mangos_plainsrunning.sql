ALTER TABLE db_version CHANGE COLUMN required_s2492_01_mangos_gobject_near_link required_s2493_01_mangos_plainsrunning bit;

DELETE FROM spell_template WHERE Id = 90001;
INSERT INTO spell_template
    (Id, Attributes, ProcChance, DurationIndex, RangeIndex,
     Effect1, EffectDieSides1, EffectBaseDice1, EffectBasePoints1,
     EffectImplicitTargetA1, EffectApplyAuraName1, DmgMultiplier1,
     SpellName, IsServerSide)
VALUES
    (90001, 0, 101, 21, 1,
     6, 1, 1, 29,
     1, 31, 1,
     'Plainsrunning', 1);
