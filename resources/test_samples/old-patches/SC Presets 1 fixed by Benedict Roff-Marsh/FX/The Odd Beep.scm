<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="The Odd Beep" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Bell C2.wav" key_root="48" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.003000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43826" loop_start="0" loop_end="43826" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000001" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\bell c2.wav">
            <filter identifier="0" type="LPHPS" p0="-7.000000" p1="0.900000" p2="-6.350000" p3="0.900000" p4="5.300000" />
            <filter identifier="1" type="PMOD" p1="24.000000" p2="4.500000" p3="6.000000" p4="-11.600001" />
            <envelope identifier="0" attack="-4.100000" attack_shape="0.025000" hold="-10.000000" decay="3.400000" decay_shape="-1.040000" sustain="1.000000" release="-3.900000" release_shape="1.050000" />
            <envelope identifier="1" attack="-5.300000" attack_shape="0.980000" hold="-10.000000" decay="-1.400000" decay_shape="0.000000" sustain="0.000000" release="0.900000" release_shape="0.000000" />
            <lfo identifier="0" rate="-2.800000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.098422" step1="-0.295755" step2="-0.885922" step3="0.215369" step4="0.566637" step5="0.605213" step6="0.039766" step7="-0.396100" step8="0.751946" step9="0.453352" step10="0.911802" step11="0.851436" step12="0.078707" step13="-0.715323" step14="-0.075838" step15="-0.529344" step16="0.724479" step17="-0.580798" step18="0.559313" step19="0.687307" step20="0.993591" step21="0.999390" step22="0.222999" step23="-0.215125" step24="-0.467574" step25="-0.405438" step26="0.680288" step27="-0.952513" step28="-0.248268" step29="-0.814753" step30="0.354411" step31="-0.887570" />
            <lfo identifier="1" rate="-0.560000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="keytrack" dest="f1p1" amount="0.997500" />
            <modulation identifier="1" src="keytrack" dest="f1p3" amount="1.000000" />
            <modulation identifier="2" src="keytrack" dest="f2p1" amount="0.004999" />
            <modulation identifier="3" src="stepLFO1" dest="f2p1" amount="128.000000" />
            <modulation identifier="4" src="modwheel" dest="f1p1" amount="12.000000" />
            <modulation identifier="5" src="modwheel" dest="f1p3" amount="12.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
