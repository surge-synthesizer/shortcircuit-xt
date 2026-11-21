<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Step This Way" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="NONE" p0="-1.630000" p3="0.320000" p4="5.300000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p1="1.000000" p2="4.500000" p3="12.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-10.000000" decay="3.800000" decay_shape="0.020000" sustain="0.000000" release="-4.500000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.980000" hold="-10.000000" decay="3.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.880000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-0.960000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step0="0.038961" step1="0.012987" step2="0.090909" step3="0.038961" step4="0.090909" step5="0.038961" step6="0.038961" step7="0.038961" step8="0.038961" step9="0.012987" step10="-0.038961" step11="0.012987" step12="0.012987" step13="0.038961" step14="0.038961" step15="0.038961" step16="0.038961" step17="0.038961" step18="0.064935" step19="0.038961" step20="0.090909" step21="0.090909" step22="0.012987" step23="-0.012987" step24="0.064935" step25="0.064935" step26="0.012987" step27="0.064935" step28="0.064935" step29="0.038961" step30="0.064935" step31="0.064935" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="7.000000" />
            <modulation identifier="1" src="EG2" dest="f2p1" amount="5.000000" />
            <modulation identifier="2" src="stepLFO2" dest="pitch" amount="0.044999" />
            <modulation identifier="3" src="EG2" dest="mm2" amount="-1.500000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Saw A C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="12" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43152" loop_start="0" loop_end="43152" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\saw a c2.wav">
            <filter identifier="0" type="NONE" p0="-1.630000" p3="0.320000" p4="5.300000" />
            <filter identifier="1" type="LP4Msat" p0="-7.000000" p2="4.500000" p4="-11.600001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.025000" hold="-10.000000" decay="3.800000" decay_shape="0.020000" sustain="0.000000" release="-4.500000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.980000" hold="-10.000000" decay="3.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="1.880000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-0.960000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step0="0.038961" step1="0.012987" step2="0.090909" step3="0.038961" step4="0.090909" step5="0.038961" step6="0.038961" step7="0.038961" step8="0.038961" step9="0.012987" step10="-0.038961" step11="0.012987" step12="0.012987" step13="0.038961" step14="0.038961" step15="0.038961" step16="0.038961" step17="0.038961" step18="0.064935" step19="0.038961" step20="0.090909" step21="0.090909" step22="0.012987" step23="-0.012987" step24="0.064935" step25="0.064935" step26="0.012987" step27="0.064935" step28="0.064935" step29="0.038961" step30="0.064935" step31="0.064935" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="modwheel" dest="f2p1" amount="7.000000" />
            <modulation identifier="1" src="EG2" dest="f2p1" amount="5.000000" />
            <modulation identifier="2" src="stepLFO2" dest="pitch" amount="0.044999" />
            <modulation identifier="3" src="EG2" dest="mm2" amount="-1.500000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
