<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Off With Her Head" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Saw A C2.wav" key_root="36" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="OSCPSY" p0="-11.975000" p1="0.000001" p4="36.000000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p1="0.150000" p2="2.300000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-9.099999" decay="-10.000000" decay_shape="0.020000" sustain="0.500000" release="-10.000000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-0.700000" decay_shape="0.000000" sustain="0.000000" release="-0.700000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.800000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step1="-0.062500" step2="-0.125000" step3="-0.187500" step4="-0.250000" step5="-0.312500" step6="-0.375000" step7="-0.437500" step8="-0.500000" step9="-0.562500" step10="-0.625000" step11="-0.687500" step12="-0.750000" step13="-0.812500" step14="-0.875000" step15="-0.937500" step16="-1.000000" step17="-0.937500" step18="-0.875000" step19="-0.812500" step20="-0.750000" step21="-0.687500" step22="-0.625000" step23="-0.562500" step24="-0.500000" step25="-0.437500" step26="-0.375000" step27="-0.312500" step28="-0.250000" step29="-0.187500" step30="-0.125000" step31="-0.062500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="12.000000" />
            <modulation identifier="1" src="EG2" dest="none" amount="50.000000" />
            <modulation identifier="2" src="stepLFO1" dest="pitch" amount="0.175000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
