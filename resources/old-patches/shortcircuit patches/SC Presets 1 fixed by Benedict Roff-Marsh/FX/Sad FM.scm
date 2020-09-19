<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Sad FM" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="12" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="Chorus" p1="-8.095860" p2="-1.400000" p3="0.700000" p5="-2.600000" p6="4.960000" />
        <effect identifier="1" type="DigiD" p0="-7.050000" p1="-1.036965" p2="-12.000000" p3="-2.890000" p4="4.450000" p5="-1.000000" p6="4.000000" />
        <grouplfo rate="2.400000" smooth="0.000000" syncmode="0" repeat="16" />
        <modulation identifier="0" src="groupLFO" dest="none" amount="0.035000" />
        <modulation identifier="1" src="groupLFO" dest="none" amount="2.000000" />
        <zone name="Sine C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="PMOD" p1="-0.000001" p2="4.000000" p3="3.599994" />
            <filter identifier="1" type="PMOD" p0="-12.000000" p1="-12.000001" p2="6.600000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="0.000000" decay_shape="0.040000" sustain="0.000000" release="0.000000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.221928" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="0.000000" syncmode="9" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-1.720000" smooth="0.000000" syncmode="13" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-0.994000" smooth="0.000000" syncmode="1" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="f1p1" amount="2.000000" />
            <modulation identifier="1" src="keytrack" dest="f1p1" amount="0.500000" />
            <modulation identifier="3" src="modwheel" dest="f1p1" amount="12.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
