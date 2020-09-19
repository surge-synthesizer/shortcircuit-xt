<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Fear" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="-6.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="FSflange" p0="-3.000000" p1="-3.000000" p2="0.020000" p3="6.980000" p4="6.000000" p5="-1.000000" p6="4.000000" />
        <effect identifier="1" type="Chorus" p0="-6.000000" p1="-4.100000" p2="-0.800000" p3="0.120000" p4="1.000000" p6="4.000000" />
        <grouplfo rate="2.640000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="12" mute_group="0" polymode="1" portamode="0" portamento="-1.900000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.500000" lag1="2.200000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="OD" p0="0.700000" p1="-2.360000" p2="7.050000" p3="-7.000000" p4="1.000000" />
            <filter identifier="1" type="NONE" p0="0.035000" p1="-144.000000" />
            <envelope identifier="0" attack="-3.200000" attack_shape="0.000000" hold="-10.000000" decay="2.807355" decay_shape="-0.963000" sustain="1.000000" release="-2.580822" release_shape="1.000000" />
            <envelope identifier="1" attack="-0.200000" attack_shape="0.000000" hold="-9.800000" decay="2.300000" decay_shape="0.000000" sustain="0.000000" release="-1.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="-2.240000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.393902" step1="-0.146886" step2="-0.859249" step3="0.933226" step4="0.366375" step5="-0.693533" step6="0.754509" step7="0.643361" step8="0.164098" step9="-0.617298" step10="-0.644215" step11="0.634388" step12="-0.049471" step13="-0.688894" step14="0.007843" step15="0.464034" step16="-0.188818" step17="-0.440840" step18="0.137486" step19="0.364483" step20="0.511704" step21="0.443831" step22="-0.049409" step23="-0.753960" step24="-0.264382" step25="0.669362" step26="-0.929807" step27="0.034028" step28="0.325968" step29="-0.147557" step30="-0.790643" step31="0.898679" />
            <lfo identifier="1" rate="-0.480000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-1.520000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p4" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="none" amount="1.049993" />
            <modulation identifier="2" src="keytrack" dest="none" amount="-1.960000" />
            <modulation identifier="3" src="keytrack" dest="none" amount="-0.190000" />
            <modulation identifier="4" src="stepLFO1" dest="f1p4" amount="0.203500" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.490000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="12" mute_group="0" polymode="1" portamode="0" portamento="-1.900000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.500000" lag1="2.200000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="OD" p0="0.700000" p1="-2.360000" p2="7.050000" p3="-7.000000" p4="1.000000" />
            <filter identifier="1" type="NONE" p0="0.035000" p1="-144.000000" />
            <envelope identifier="0" attack="-3.200000" attack_shape="0.000000" hold="-10.000000" decay="2.807355" decay_shape="-0.963000" sustain="1.000000" release="-2.580822" release_shape="1.000000" />
            <envelope identifier="1" attack="-0.200000" attack_shape="0.000000" hold="-9.800000" decay="2.300000" decay_shape="0.000000" sustain="0.000000" release="-1.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="-2.240000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.393902" step1="-0.146886" step2="-0.859249" step3="0.933226" step4="0.366375" step5="-0.693533" step6="0.754509" step7="0.643361" step8="0.164098" step9="-0.617298" step10="-0.644215" step11="0.634388" step12="-0.049471" step13="-0.688894" step14="0.007843" step15="0.464034" step16="-0.188818" step17="-0.440840" step18="0.137486" step19="0.364483" step20="0.511704" step21="0.443831" step22="-0.049409" step23="-0.753960" step24="-0.264382" step25="0.669362" step26="-0.929807" step27="0.034028" step28="0.325968" step29="-0.147557" step30="-0.790643" step31="0.898679" />
            <lfo identifier="1" rate="-0.480000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-1.520000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f1p4" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="none" amount="1.049993" />
            <modulation identifier="2" src="keytrack" dest="none" amount="-1.960000" />
            <modulation identifier="3" src="keytrack" dest="none" amount="-0.190000" />
            <modulation identifier="4" src="stepLFO1" dest="f1p4" amount="0.203500" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="128" />
</shortcircuit>
