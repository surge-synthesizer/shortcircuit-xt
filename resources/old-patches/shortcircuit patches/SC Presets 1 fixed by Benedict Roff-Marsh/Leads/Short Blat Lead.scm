<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Short Blat Lead" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-7.200000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="OD" p1="1.010000" p3="-7.000000" />
            <filter identifier="1" type="NONE" p4="19.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="-10.000000" hold="-10.000000" decay="-0.500000" decay_shape="1.010000" sustain="0.000000" release="-10.000000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-0.700000" decay_shape="0.000000" sustain="0.000000" release="-0.700000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.240000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.174291" step1="0.098666" step2="0.066195" step3="0.246437" step4="0.331254" step5="0.178234" step6="0.199255" step7="0.051595" step8="-0.055818" step9="0.129868" step10="0.345500" step11="0.091317" step12="0.143553" step13="0.114463" step14="0.028803" step15="-0.025385" step16="0.160265" step17="0.065181" step18="-0.028291" step19="-0.232252" step20="-0.305777" step21="-0.213587" step22="-0.151280" step23="0.184094" step24="0.394671" step25="0.362835" step26="0.109275" step27="0.165355" step28="-0.227125" step29="-0.160228" step30="-0.188098" step31="-0.146532" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p4" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="f1p4" amount="3.000000" />
            <modulation identifier="2" src="stepLFO1" dest="pitch" amount="1.025000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000500" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-7.200000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="OD" p1="0.310000" p2="12.600000" p3="-7.000000" p4="0.600000" />
            <filter identifier="1" type="NONE" p4="19.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="-0.370000" hold="-10.000000" decay="-0.500000" decay_shape="1.010000" sustain="0.000000" release="-10.000000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.100000" decay_shape="0.000000" sustain="0.000000" release="-3.100000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.240000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.174291" step1="0.098666" step2="0.066195" step3="0.246437" step4="0.331254" step5="0.178234" step6="0.199255" step7="0.051595" step8="-0.055818" step9="0.129868" step10="0.345500" step11="0.091317" step12="0.143553" step13="0.114463" step14="0.028803" step15="-0.025385" step16="0.160265" step17="0.065181" step18="-0.028291" step19="-0.232252" step20="-0.305777" step21="-0.213587" step22="-0.151280" step23="0.184094" step24="0.394671" step25="0.362835" step26="0.109275" step27="0.165355" step28="-0.227125" step29="-0.160228" step30="-0.188098" step31="-0.146532" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p4" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="f1p4" amount="3.000000" />
            <modulation identifier="2" src="stepLFO1" dest="pitch" amount="1.025000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
