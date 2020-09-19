<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.1.2" revision="9">
    <group is_root_group="yes">
        <zone name="tremolo" key_root="5" key_low="42" key_high="53" lo_vel="0" hi_vel="127" key_low_fade="0" key_high_fade="0" lo_vel_fade="0" hi_vel_fade="0" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="512" sample_end="512" loop_start="0" loop_end="512" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\simple modulation examples samples\sine512.wav">
            <filter identifier="0" type="HP2A" bypass="no" p0="1.110000" />
            <filter identifier="1" type="NONE" bypass="no" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-10.000000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.160000" smooth="1.000000" syncmode="0" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="stepLFO1" dest="amplitude" amount="1.950000" />
            <slice identifier="0" samplepos="0" end="0" env="0.000000" mute="no" />
            <slice identifier="1" samplepos="256" end="256" env="0.000000" mute="no" />
            <slice identifier="2" samplepos="257" end="257" env="0.000000" mute="no" />
        </zone>
        <zone name="vibrato 1" key_root="5" key_low="54" key_high="68" lo_vel="0" hi_vel="127" key_low_fade="0" key_high_fade="0" lo_vel_fade="0" hi_vel_fade="0" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="512" loop_start="0" loop_end="512" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\simple modulation examples samples\sine512.wav">
            <filter identifier="0" type="HP2A" bypass="no" p0="1.090000" />
            <filter identifier="1" type="NONE" bypass="no" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.600000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step1="-0.062500" step2="-0.125000" step3="-0.187500" step4="-0.250000" step5="-0.312500" step6="-0.375000" step7="-0.437500" step8="-0.500000" step9="-0.562500" step10="-0.625000" step11="-0.687500" step12="-0.750000" step13="-0.812500" step14="-0.875000" step15="-0.937500" step16="-1.000000" step17="-0.937500" step18="-0.875000" step19="-0.812500" step20="-0.750000" step21="-0.687500" step22="-0.625000" step23="-0.562500" step24="-0.500000" step25="-0.437500" step26="-0.375000" step27="-0.312500" step28="-0.250000" step29="-0.187500" step30="-0.125000" step31="-0.062500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="stepLFO1" dest="rate" amount="0.040001" />
            <slice identifier="0" samplepos="0" end="512" env="0.000000" mute="no" />
        </zone>
        <zone name="vibrato 2" key_root="17" key_low="69" key_high="83" lo_vel="0" hi_vel="127" key_low_fade="0" key_high_fade="0" lo_vel_fade="0" hi_vel_fade="0" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="512" loop_start="0" loop_end="512" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\simple modulation examples samples\sine512.wav">
            <filter identifier="0" type="HP2A" bypass="no" p0="1.140000" />
            <filter identifier="1" type="NONE" bypass="no" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="2.600000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step1="0.125000" step2="0.250000" step3="0.375000" step4="0.500000" step5="0.625000" step6="0.750000" step7="0.875000" step8="1.000000" step9="0.875000" step10="0.750000" step11="0.625000" step12="0.500000" step13="0.375000" step14="0.250000" step15="0.125000" step17="-0.125000" step18="-0.250000" step19="-0.375000" step20="-0.500000" step21="-0.625000" step22="-0.750000" step23="-0.875000" step24="-1.000000" step25="-0.875000" step26="-0.750000" step27="-0.625000" step28="-0.500000" step29="-0.375000" step30="-0.250000" step31="-0.125000" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="stepLFO1" dest="rate" amount="0.020001" />
            <slice identifier="0" samplepos="0" end="512" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="0" poly_cap="256" />
</shortcircuit>
