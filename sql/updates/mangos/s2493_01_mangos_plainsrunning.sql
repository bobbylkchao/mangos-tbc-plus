ALTER TABLE db_version CHANGE COLUMN required_s2492_01_mangos_gobject_near_link required_s2493_01_mangos_plainsrunning bit;

UPDATE spell_template SET
    Attributes = 0,
    AttributesEx = 0,
    AttributesEx2 = 0,
    AttributesEx3 = 0,
    AttributesEx4 = 0,
    AttributesEx5 = 0,
    DurationIndex = 21,
    RangeIndex = 1,
    StackAmount = 30,
    Effect1 = 6,
    Effect2 = 0,
    Effect3 = 0,
    EffectDieSides1 = 1,
    EffectBaseDice1 = 1,
    EffectBasePoints1 = 0,
    EffectImplicitTargetA1 = 1,
    EffectImplicitTargetB1 = 0,
    EffectApplyAuraName1 = 31,
    EffectMiscValue1 = 0,
    EffectMiscValueB1 = 0,
    EffectTriggerSpell1 = 0,
    DmgMultiplier1 = 1
WHERE Id = 12821;
