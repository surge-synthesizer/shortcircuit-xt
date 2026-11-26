<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Bo Solo" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="12" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="DigiD" p0="-18.000000" p1="-0.100000" p2="-21.000000" p3="-2.000000" p4="1.230000" p5="-1.000000" p6="4.000000" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Square C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003500" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-5.321928" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\square c2.wav">
            <filter identifier="0" type="NONE" p0="73.050003" p1="-10.950002" p2="4.000000" />
            <filter identifier="1" type="BF" p0="6.647131" p1="1.000000" p3="-7.000000" p4="0.600000" />
            <envelope identifier="0" attack="-2.321928" attack_shape="0.000000" hold="-10.000000" decay="1.584962" decay_shape="1.000000" sustain="1.000000" release="-2.321928" release_shape="1.000000" />
            <envelope identifier="1" attack="0.000000" attack_shape="0.000000" hold="-10.000000" decay="1.000000" decay_shape="0.000000" sustain="0.000000" release="1.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.584962" smooth="1.000000" syncmode="0" keytrigger="no" repeat="2" step0="1.000000" step1="-1.000000" step2="0.875000" step3="0.812500" step4="0.750000" step5="0.687500" step6="0.625000" step7="0.562500" step8="0.500000" step9="0.437500" step10="0.375000" step11="0.312500" step12="0.250000" step13="0.187500" step14="0.125000" step15="0.062500" step17="-0.062500" step18="-0.125000" step19="-0.187500" step20="-0.250000" step21="-0.312500" step22="-0.375000" step23="-0.437500" step24="-0.500000" step25="-0.562500" step26="-0.625000" step27="-0.687500" step28="-0.750000" step29="-0.812500" step30="-0.875000" step31="-0.937500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p4" amount="11.000000" />
            <modulation identifier="1" src="EG2" dest="f2p4" amount="1.000000" />
            <modulation identifier="2" src="stepLFO1" dest="pitch" amount="0.130000" />
            <modulation identifier="3" src="keytrack" dest="f1p1" amount="1.000000" />
            <modulation identifier="4" src="keytrack" dest="f2p4" amount="0.500000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.103500" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-5.321928" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="NONE" p0="73.050003" p1="-10.950002" p2="4.000000" />
            <filter identifier="1" type="BF" p0="6.647131" p1="1.000000" p3="-7.000000" p4="0.600000" />
            <envelope identifier="0" attack="-2.321928" attack_shape="0.000000" hold="-10.000000" decay="1.584962" decay_shape="1.000000" sustain="1.000000" release="-2.321928" release_shape="1.000000" />
            <envelope identifier="1" attack="0.000000" attack_shape="0.000000" hold="-10.000000" decay="1.000000" decay_shape="0.000000" sustain="0.000000" release="1.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.584962" smooth="1.000000" syncmode="0" keytrigger="no" repeat="2" step0="1.000000" step1="-1.000000" step2="0.875000" step3="0.812500" step4="0.750000" step5="0.687500" step6="0.625000" step7="0.562500" step8="0.500000" step9="0.437500" step10="0.375000" step11="0.312500" step12="0.250000" step13="0.187500" step14="0.125000" step15="0.062500" step17="-0.062500" step18="-0.125000" step19="-0.187500" step20="-0.250000" step21="-0.312500" step22="-0.375000" step23="-0.437500" step24="-0.500000" step25="-0.562500" step26="-0.625000" step27="-0.687500" step28="-0.750000" step29="-0.812500" step30="-0.875000" step31="-0.937500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p4" amount="11.000000" />
            <modulation identifier="1" src="EG2" dest="f2p4" amount="1.000000" />
            <modulation identifier="2" src="stepLFO1" dest="pitch" amount="0.130000" />
            <modulation identifier="3" src="keytrack" dest="f1p1" amount="1.000000" />
            <modulation identifier="4" src="keytrack" dest="f2p4" amount="0.500000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
