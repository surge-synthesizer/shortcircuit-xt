<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Varichord" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="DigiD" p0="-12.000000" p1="-3.300000" p2="-12.000000" p3="-2.680000" p4="5.190000" p5="-1.000000" p6="4.000000" />
        <effect identifier="1" type="NONE" p0="-15.000000" p1="-1.000000" p2="-15.000000" p3="-2.000000" p4="4.000000" p5="-1.000000" />
        <grouplfo rate="2.640000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Triangle C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="0" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\triangle c2.wav">
            <filter identifier="0" type="PMOD" p0="-0.000000" p1="-12.000000" />
            <filter identifier="1" type="NONE" p0="0.035000" p1="-144.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="0.000000" sustain="0.000000" release="-7.880822" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.900000" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-0.440000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-0.480000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-1.520000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p2" amount="24.000000" />
            <modulation identifier="1" src="EG2" dest="f1p2" amount="5.999993" />
            <modulation identifier="2" src="keytrack" dest="f1p2" amount="-1.960000" />
            <modulation identifier="3" src="keytrack" dest="eg1d" amount="-0.190000" />
            <modulation identifier="4" src="pb_up" dest="eg1r" amount="8.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="128" />
</shortcircuit>
