<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="With Luck" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Square C2.wav" key_root="36" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-4.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\square c2.wav">
            <filter identifier="0" type="NONE" p0="12.000000" p3="0.320000" p4="5.300000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p1="0.500000" p2="4.500000" p3="6.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-10.000000" decay="3.400000" decay_shape="-1.040000" sustain="1.000000" release="-10.000000" release_shape="1.050000" />
            <envelope identifier="1" attack="-5.300000" attack_shape="0.980000" hold="-10.000000" decay="-1.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.880000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="-0.560000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="f2p1" amount="1.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p1" amount="0.254999" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="0.100000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Square C2.wav" key_root="36" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.053000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="2" portamode="0" portamento="-4.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\square c2.wav">
            <filter identifier="0" type="NONE" p0="12.000000" p3="0.320000" p4="5.300000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p1="0.500000" p2="4.500000" p3="6.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-10.000000" decay="3.400000" decay_shape="-1.040000" sustain="1.000000" release="-10.000000" release_shape="1.050000" />
            <envelope identifier="1" attack="-5.300000" attack_shape="0.980000" hold="-10.000000" decay="-1.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.880000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="-0.560000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="f2p1" amount="1.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p1" amount="0.254999" />
            <modulation identifier="3" src="stepLFO1" dest="pitch" amount="0.100000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
