<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Droid Chatter" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Bell C2.wav" key_root="36" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\bell c2.wav">
            <filter identifier="0" type="NONE" p0="-1.630000" p3="0.320000" p4="5.300000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p1="1.000000" p2="4.500000" p3="12.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-10.000000" decay="3.800000" decay_shape="0.020000" sustain="1.000000" release="-4.500000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.980000" hold="-10.000000" decay="3.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.880000" smooth="0.000000" syncmode="0" keytrigger="yes" repeat="32" step0="1.000000" step1="1.000000" step2="1.000000" step3="1.000000" step4="1.000000" step5="1.000000" step6="1.000000" step7="1.000000" step8="1.000000" step9="1.000000" step10="1.000000" step11="1.000000" step12="1.000000" step13="1.000000" step14="1.000000" step15="1.000000" step16="-1.000000" step17="-1.000000" step18="-1.000000" step19="-1.000000" step20="-1.000000" step21="-1.000000" step22="-1.000000" step23="-1.000000" step24="-1.000000" step25="-1.000000" step26="-1.000000" step27="-1.000000" step28="-1.000000" step29="-1.000000" step30="-1.000000" step31="-1.000000" />
            <lfo identifier="1" rate="-0.560000" smooth="0.400000" syncmode="0" keytrigger="yes" repeat="32" step0="-0.753960" step1="-0.264382" step2="0.669362" step3="-0.929807" step4="0.034028" step5="0.325968" step6="-0.147557" step7="-0.790643" step8="0.898679" step9="0.842769" step10="0.099094" step11="-0.308023" step12="-0.056551" step13="-0.250038" step14="0.693960" step15="-0.366253" step16="-0.087802" step17="-0.456221" step18="0.965941" step19="-0.404401" step20="0.478378" step21="0.134556" step22="-0.608020" step23="0.522629" step24="0.678884" step25="-0.204688" step26="0.001801" step27="0.780328" step28="-0.945067" step29="0.989257" step30="0.145177" step31="-0.898984" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="f2p1" amount="5.000000" />
            <modulation identifier="2" src="stepLFO2" dest="pitch" amount="-11.955001" />
            <modulation identifier="3" src="EG2" dest="mm2" amount="32.000000" />
            <modulation identifier="4" src="keytrack" dest="f1p1" amount="1.000000" />
            <modulation identifier="5" src="stepLFO1" dest="pan" amount="0.100000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
