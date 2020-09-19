<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Crazy Man" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Bell C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000004" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\bell c2.wav">
            <filter identifier="0" type="BF" p0="4.850000" p1="0.550000" p3="4.570000" />
            <filter identifier="1" type="RING" p0="-0.250000" p2="4.500000" p3="6.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-4.100000" attack_shape="0.025000" hold="-10.000000" decay="3.400000" decay_shape="-1.040000" sustain="1.000000" release="-3.900000" release_shape="1.050000" />
            <envelope identifier="1" attack="-5.300000" attack_shape="0.980000" hold="-10.000000" decay="-1.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.060000" smooth="0.500000" syncmode="0" keytrigger="no" repeat="32" step0="-0.142857" step1="0.220779" step2="0.532468" step3="0.350649" step4="-0.012987" step5="0.012987" step6="0.038961" step7="0.220779" step8="0.142857" step9="-0.168831" step10="-0.272727" step11="-0.090909" step12="0.090909" step13="-0.064935" step14="0.168831" step15="0.402597" step16="0.012987" step17="0.090909" step18="0.142857" step19="-0.246753" step20="-0.064935" step21="0.116883" step22="0.324675" step23="0.038961" step24="0.298701" step25="-0.064935" step26="0.142857" step27="0.324675" step28="0.064935" step29="0.246753" step30="0.194805" step31="0.012987" />
            <lfo identifier="1" rate="-1.920000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="lfo1rate" amount="6.000000" />
            <modulation identifier="3" src="stepLFO1" dest="f2p1" amount="7.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
