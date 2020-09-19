<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Alarm" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" p1="-8.600000" p2="-0.800000" p3="0.120000" p4="1.000000" p5="-2.890000" p6="5.170000" />
        <effect identifier="1" type="NONE" p1="4.700000" p2="-2.800000" p3="0.120000" p4="1.000000" p5="-2.950000" p6="6.000000" />
        <envelope identifier="0" mode="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.300000" decay_shape="0.000000" sustain="1.000000" release="-10.000000" release_shape="0.000000" />
        <grouplfo rate="-2.880000" smooth="1.000000" syncmode="0" repeat="32" step1="0.125000" step2="0.250000" step3="0.375000" step4="0.500000" step5="0.625000" step6="0.750000" step7="0.875000" step8="1.000000" step9="0.875000" step10="0.750000" step11="0.625000" step12="0.500000" step13="0.375000" step14="0.250000" step15="0.125000" step17="-0.125000" step18="-0.250000" step19="-0.375000" step20="-0.500000" step21="-0.625000" step22="-0.750000" step23="-0.875000" step24="-1.000000" step25="-0.875000" step26="-0.750000" step27="-0.625000" step28="-0.500000" step29="-0.375000" step30="-0.250000" step31="-0.125000" />
        <modulation identifier="0" src="groupLFO" dest="pitch" amount="0.040000" />
        <modulation identifier="1" src="groupLFO" dest="pan" amount="0.999500" />
        <zone name="Triangle C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\triangle c2.wav">
            <filter identifier="0" type="DIST1" p0="6.000000" p1="-3.020000" p3="-7.000000" />
            <filter identifier="1" type="OD" p0="0.100000" p1="0.500000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="1.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="0.000000" release="-7.600000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.321928" decay_shape="0.000000" sustain="0.000000" release="-2.300000" release_shape="0.000000" />
            <lfo identifier="0" rate="-0.360000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="8" step0="-1.000000" step1="-0.532468" step3="0.558442" step4="1.000000" step5="0.558442" step7="-0.532468" />
            <lfo identifier="1" rate="-1.000000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step0="1.000000" step1="0.948052" step2="0.922078" step3="0.870130" step4="0.844156" step5="0.818182" step6="0.766234" step7="0.688312" step8="0.636364" step9="0.610390" step10="0.584416" step11="0.532468" step12="0.506494" step13="0.506494" step14="0.454545" step15="0.428571" step16="0.428571" step17="0.376623" step18="0.324675" step19="0.298701" step20="0.246753" step21="0.246753" step22="0.220779" step23="0.194805" step24="0.194805" step25="0.168831" step26="0.142857" step27="0.090909" step28="0.090909" step29="0.064935" step30="0.038961" step31="0.038961" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="f2p4" amount="0.200000" />
            <modulation identifier="1" src="modwheel" dest="f2p4" amount="12.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p4" amount="0.250000" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="10.900001" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Triangle C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.110000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\triangle c2.wav">
            <filter identifier="0" type="DIST1" p0="6.000000" p1="-3.020000" p3="-7.000000" />
            <filter identifier="1" type="OD" p0="0.100000" p1="0.500000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="1.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="0.000000" release="-7.600000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.321928" decay_shape="0.000000" sustain="0.000000" release="-2.300000" release_shape="0.000000" />
            <lfo identifier="0" rate="-0.360000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="8" step0="-1.000000" step1="-0.532468" step3="0.558442" step4="1.000000" step5="0.558442" step7="-0.532468" />
            <lfo identifier="1" rate="-1.000000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step0="1.000000" step1="0.948052" step2="0.922078" step3="0.870130" step4="0.844156" step5="0.818182" step6="0.766234" step7="0.688312" step8="0.636364" step9="0.610390" step10="0.584416" step11="0.532468" step12="0.506494" step13="0.506494" step14="0.454545" step15="0.428571" step16="0.428571" step17="0.376623" step18="0.324675" step19="0.298701" step20="0.246753" step21="0.246753" step22="0.220779" step23="0.194805" step24="0.194805" step25="0.168831" step26="0.142857" step27="0.090909" step28="0.090909" step29="0.064935" step30="0.038961" step31="0.038961" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="f2p4" amount="0.200000" />
            <modulation identifier="1" src="modwheel" dest="f2p4" amount="12.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p4" amount="0.250000" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="10.900001" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Triangle C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="24" finetune="-0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\triangle c2.wav">
            <filter identifier="0" type="DIST1" p0="6.000000" p1="-3.020000" p3="-7.000000" />
            <filter identifier="1" type="OD" p0="0.100000" p1="0.500000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="1.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="0.000000" release="-7.600000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.321928" decay_shape="0.000000" sustain="0.000000" release="-2.300000" release_shape="0.000000" />
            <lfo identifier="0" rate="-0.360000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="8" step0="-1.000000" step1="-0.532468" step3="0.558442" step4="1.000000" step5="0.558442" step7="-0.532468" />
            <lfo identifier="1" rate="-1.000000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step0="1.000000" step1="0.948052" step2="0.922078" step3="0.870130" step4="0.844156" step5="0.818182" step6="0.766234" step7="0.688312" step8="0.636364" step9="0.610390" step10="0.584416" step11="0.532468" step12="0.506494" step13="0.506494" step14="0.454545" step15="0.428571" step16="0.428571" step17="0.376623" step18="0.324675" step19="0.298701" step20="0.246753" step21="0.246753" step22="0.220779" step23="0.194805" step24="0.194805" step25="0.168831" step26="0.142857" step27="0.090909" step28="0.090909" step29="0.064935" step30="0.038961" step31="0.038961" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="f2p4" amount="0.200000" />
            <modulation identifier="1" src="modwheel" dest="f2p4" amount="12.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p4" amount="0.250000" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="10.900001" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Triangle C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="24" finetune="-0.100000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\triangle c2.wav">
            <filter identifier="0" type="DIST1" p0="6.000000" p1="-3.020000" p3="-7.000000" />
            <filter identifier="1" type="OD" p0="0.100000" p1="0.500000" p3="-7.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="1.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="0.000000" release="-7.600000" release_shape="1.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.321928" decay_shape="0.000000" sustain="0.000000" release="-2.300000" release_shape="0.000000" />
            <lfo identifier="0" rate="-0.360000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="8" step0="-1.000000" step1="-0.532468" step3="0.558442" step4="1.000000" step5="0.558442" step7="-0.532468" />
            <lfo identifier="1" rate="-1.000000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step0="1.000000" step1="0.948052" step2="0.922078" step3="0.870130" step4="0.844156" step5="0.818182" step6="0.766234" step7="0.688312" step8="0.636364" step9="0.610390" step10="0.584416" step11="0.532468" step12="0.506494" step13="0.506494" step14="0.454545" step15="0.428571" step16="0.428571" step17="0.376623" step18="0.324675" step19="0.298701" step20="0.246753" step21="0.246753" step22="0.220779" step23="0.194805" step24="0.194805" step25="0.168831" step26="0.142857" step27="0.090909" step28="0.090909" step29="0.064935" step30="0.038961" step31="0.038961" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="f2p4" amount="0.200000" />
            <modulation identifier="1" src="modwheel" dest="f2p4" amount="12.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p4" amount="0.250000" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="10.900001" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
