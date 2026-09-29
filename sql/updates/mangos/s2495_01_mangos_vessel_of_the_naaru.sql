ALTER TABLE db_version CHANGE COLUMN required_s2494_01_mangos_craftsman_board required_s2495_01_mangos_vessel_of_the_naaru bit;

-- Vial of the Sunwell / Vessel of the Naaru (45059) procs Holy Energy (45062) from healing.
-- Without suppressing caster procs on 45062, each Holy Energy stack re-triggers 45059 and
-- overflows MaxSpellCastsInChain ("too deep in cast chain").
UPDATE `spell_template`
SET `AttributesEx3` = (`AttributesEx3` | 0x00010000) -- SPELL_ATTR_EX3_SUPPRESS_CASTER_PROCS
WHERE `Id` = 45062;
