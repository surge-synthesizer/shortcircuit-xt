<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Line Choir" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="DigiD" p0="-12.000000" p1="-0.400000" p2="-12.000000" p3="-2.680000" p4="5.190000" p5="-1.000000" p6="4.000000" />
        <effect identifier="1" type="Chorus" p1="-6.700000" p2="-0.800000" p3="0.120000" p4="1.000000" p5="-0.410000" p6="5.090000" />
        <grouplfo rate="2.640000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Sine C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="12" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="0" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.500000" lag1="2.200000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="PMOD" p0="12.000000" p1="-12.000000" />
            <filter identifier="1" type="NONE" p0="0.035000" p1="-144.000000" />
            <envelope identifier="0" attack="-3.200000" attack_shape="0.000000" hold="-10.000000" decay="2.807355" decay_shape="-0.963000" sustain="0.000000" release="-2.580822" release_shape="1.000000" />
            <envelope identifier="1" attack="-0.200000" attack_shape="0.000000" hold="-9.800000" decay="2.300000" decay_shape="0.000000" sustain="0.000000" release="-1.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.800000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step1="0.125000" step2="0.250000" step3="0.375000" step4="0.500000" step5="0.625000" step6="0.750000" step7="0.875000" step8="1.000000" step9="0.875000" step10="0.750000" step11="0.625000" step12="0.500000" step13="0.375000" step14="0.250000" step15="0.125000" step17="-0.125000" step18="-0.250000" step19="-0.375000" step20="-0.500000" step21="-0.625000" step22="-0.750000" step23="-0.875000" step24="-1.000000" step25="-0.875000" step26="-0.750000" step27="-0.625000" step28="-0.500000" step29="-0.375000" step30="-0.250000" step31="-0.125000" />
            <lfo identifier="1" rate="-0.480000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-1.520000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p2" amount="24.000000" />
            <modulation identifier="1" src="EG2" dest="f1p2" amount="1.049993" />
            <modulation identifier="2" src="keytrack" dest="f1p2" amount="-1.960000" />
            <modulation identifier="3" src="keytrack" dest="eg1d" amount="-0.190000" />
            <modulation identifier="4" src="stepLFO1" dest="pitch" amount="-0.070000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Sine C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="12" finetune="0.200000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="0" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.500000" lag1="2.200000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="PMOD" p0="12.000000" p1="-12.000000" />
            <filter identifier="1" type="NONE" p0="0.035000" p1="-144.000000" />
            <envelope identifier="0" attack="-3.200000" attack_shape="0.000000" hold="-10.000000" decay="2.807355" decay_shape="-0.963000" sustain="0.000000" release="-2.580822" release_shape="1.000000" />
            <envelope identifier="1" attack="-0.200000" attack_shape="0.000000" hold="-9.800000" decay="2.300000" decay_shape="0.000000" sustain="0.000000" release="-1.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.800000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step1="0.125000" step2="0.250000" step3="0.375000" step4="0.500000" step5="0.625000" step6="0.750000" step7="0.875000" step8="1.000000" step9="0.875000" step10="0.750000" step11="0.625000" step12="0.500000" step13="0.375000" step14="0.250000" step15="0.125000" step17="-0.125000" step18="-0.250000" step19="-0.375000" step20="-0.500000" step21="-0.625000" step22="-0.750000" step23="-0.875000" step24="-1.000000" step25="-0.875000" step26="-0.750000" step27="-0.625000" step28="-0.500000" step29="-0.375000" step30="-0.250000" step31="-0.125000" />
            <lfo identifier="1" rate="-0.480000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-1.520000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p2" amount="24.000000" />
            <modulation identifier="1" src="EG2" dest="f1p2" amount="1.049993" />
            <modulation identifier="2" src="keytrack" dest="f1p2" amount="-1.960000" />
            <modulation identifier="3" src="keytrack" dest="eg1d" amount="-0.190000" />
            <modulation identifier="4" src="stepLFO1" dest="pitch" amount="-0.070000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="128" />
</shortcircuit>
