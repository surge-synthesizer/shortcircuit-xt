<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Auto Bassline" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="12" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" p0="-3.000000" p1="-3.000000" p2="6.620000" p3="0.060000" p4="7.049999" p5="-1.000000" p6="4.000000" />
        <effect identifier="1" type="DigiD" p0="-18.000000" p1="-1.200000" p2="-6.000000" p3="-1.630000" p4="2.090000" p5="-1.000000" p6="4.000000" />
        <grouplfo rate="2.400000" smooth="0.000000" syncmode="0" repeat="16" />
        <modulation identifier="0" src="groupLFO" dest="none" amount="0.035000" />
        <modulation identifier="1" src="groupLFO" dest="none" amount="2.000000" />
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="BP2A" p0="-7.000000" p1="0.100000" p2="-7.000000" />
            <filter identifier="1" type="DIST1" p0="16.027502" p2="6.600000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="-5.400000" decay_shape="3.020000" sustain="1.000000" release="-10.000000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.221928" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="1.000000" syncmode="9" keytrigger="no" repeat="32" step0="-1.000000" step1="-0.980814" step2="-0.923937" step3="-0.831556" step4="-0.707221" step5="-0.555710" step6="-0.382844" step7="-0.195266" step9="0.195085" step10="0.382673" step11="0.555556" step12="0.707090" step13="0.831454" step14="0.923866" step15="0.980777" step16="1.000000" step17="0.980795" step18="0.923902" step19="0.831505" step20="0.707156" step21="0.555633" step22="0.382758" step23="0.195176" step24="0.000093" step25="-0.194994" step26="-0.382587" step27="-0.555479" step28="-0.707025" step29="-0.831402" step30="-0.923831" step31="-0.980759" />
            <lfo identifier="1" rate="-1.720000" smooth="1.000000" syncmode="13" keytrigger="no" repeat="32" step1="0.125000" step2="0.250000" step3="0.375000" step4="0.500000" step5="0.625000" step6="0.750000" step7="0.875000" step8="1.000000" step9="0.875000" step10="0.750000" step11="0.625000" step12="0.500000" step13="0.375000" step14="0.250000" step15="0.125000" step17="-0.125000" step18="-0.250000" step19="-0.375000" step20="-0.500000" step21="-0.625000" step22="-0.750000" step23="-0.875000" step24="-1.000000" step25="-0.875000" step26="-0.750000" step27="-0.625000" step28="-0.500000" step29="-0.375000" step30="-0.250000" step31="-0.125000" />
            <lfo identifier="2" rate="-0.994000" smooth="0.050000" syncmode="4" keytrigger="no" repeat="16" step2="0.538462" step3="-0.128205" step4="-0.128205" step6="1.000000" step8="-1.000000" step9="-1.000000" step11="-1.000000" step12="0.461538" step13="0.461538" step15="0.717949" step19="-0.461538" step20="-0.461538" step21="-0.461538" step22="0.333333" step23="0.333333" step24="0.128205" step25="0.564103" step26="0.564103" step28="-0.769231" step29="-0.025641" step30="-0.487179" step31="-0.487179" />
            <modulation identifier="0" src="stepLFO1" dest="f1p1" amount="2.000000" />
            <modulation identifier="1" src="stepLFO2" dest="pan" amount="1.000000" />
            <modulation identifier="2" src="stepLFO3" dest="pitch" amount="12.000004" />
            <modulation identifier="3" src="modwheel" dest="f1p1" amount="12.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
