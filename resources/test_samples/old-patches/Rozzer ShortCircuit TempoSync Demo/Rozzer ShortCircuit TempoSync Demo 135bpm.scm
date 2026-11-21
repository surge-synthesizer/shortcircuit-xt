<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0.9" revision="6">
    <group is_root_group="yes">
        <zone name="Original Loop" key_root="36" key_low="36" key_high="36" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="LPF Down" key_root="37" key_low="37" key_high="37" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="LP4M" p0="5.500000" p1="0.500000" p2="4.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="yes" repeat="16" step1="-0.062500" step2="-0.125000" step3="-0.187500" step4="-0.250000" step5="-0.312500" step6="-0.375000" step7="-0.437500" step8="-0.500000" step9="-0.562500" step10="-0.625000" step11="-0.687500" step12="-0.750000" step13="-0.812500" step14="-0.875000" step15="-0.937500" step16="1.000000" step17="0.937500" step18="0.875000" step19="0.812500" step20="0.750000" step21="0.687500" step22="0.625000" step23="0.562500" step24="0.500000" step25="0.437500" step26="0.375000" step27="0.312500" step28="0.250000" step29="0.187500" step30="0.125000" step31="0.062500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="6.079999" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="HPF Up" key_root="38" key_low="38" key_high="38" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-2.700000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="HP2A" p0="-5.000000" p1="0.500000" p2="4.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="yes" repeat="16" step1="0.062500" step2="0.125000" step3="0.187500" step4="0.250000" step5="0.312500" step6="0.375000" step7="0.437500" step8="0.500000" step9="0.562500" step10="0.625000" step11="0.687500" step12="0.750000" step13="0.812500" step14="0.875000" step15="0.937500" step16="-1.000000" step17="-0.937500" step18="-0.875000" step19="-0.812500" step20="-0.750000" step21="-0.687500" step22="-0.625000" step23="-0.562500" step24="-0.500000" step25="-0.437500" step26="-0.375000" step27="-0.312500" step28="-0.250000" step29="-0.187500" step30="-0.125000" step31="-0.062500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="9.820000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Peak LFO" key_root="39" key_low="39" key_high="39" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="2.250000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="PKAD" p0="2.260000" p1="0.370000" p2="0.520000" p3="0.560000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="4" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="2.820000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="BitCrushed" key_root="40" key_low="40" key_high="40" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="BF" p0="7.500000" p1="0.130000" p3="5.500000" p5="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="4" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="BitCrushed Mod" key_root="41" key_low="41" key_high="41" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="BF" p0="3.320000" p1="1.000000" p3="5.500000" p5="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="1.830000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Hard Distortion" key_root="42" key_low="42" key_high="42" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-20.400002" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="DIST1" p0="30.000002" p1="1.000000" p3="5.500000" p5="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="1.830000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="F&apos;king Hard Distortion" key_root="43" key_low="43" key_high="43" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-24.600002" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="DIST1" p0="60.000004" p1="1.000000" p3="5.500000" p5="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="yes" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="1.830000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Comb Filter" key_root="44" key_low="44" key_high="44" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-6.750000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="COMB1" p0="-0.310000" p1="0.510000" p2="0.160000" p3="-0.790000" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="7" keytrigger="no" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="3.550000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Comb Filter (random)" key_root="45" key_low="45" key_high="45" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-6.750000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="COMB1" p0="-0.310000" p1="0.510000" p2="0.160000" p3="-0.790000" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="32" step0="-0.997497" step1="0.127171" step2="-0.613392" step3="0.617481" step4="0.170019" step5="-0.040254" step6="-0.299417" step7="0.791925" step8="0.645680" step9="0.493210" step10="-0.651784" step11="0.717887" step12="0.421003" step13="0.027070" step14="-0.392010" step15="-0.970031" step16="-0.817194" step17="-0.271096" step18="-0.705374" step19="-0.668203" step20="0.977050" step21="-0.108615" step22="-0.761834" step23="-0.990661" step24="-0.982177" step25="-0.244240" step26="0.063326" step27="0.142369" step28="0.203528" step29="0.214331" step30="-0.667531" step31="0.326090" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="3.550000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Comb &amp; Freq Shift (random)" key_root="46" key_low="46" key_high="46" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-1.500000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="COMB1" p0="-0.310000" p1="0.700000" p2="0.160000" p3="-0.790000" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="FREQSHIFT" p1="1.000000" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="32" step0="-0.997497" step1="0.127171" step2="-0.613392" step3="0.617481" step4="0.170019" step5="-0.040254" step6="-0.299417" step7="0.791925" step8="0.645680" step9="0.493210" step10="-0.651784" step11="0.717887" step12="0.421003" step13="0.027070" step14="-0.392010" step15="-0.970031" step16="-0.817194" step17="-0.271096" step18="-0.705374" step19="-0.668203" step20="0.977050" step21="-0.108615" step22="-0.761834" step23="-0.990661" step24="-0.982177" step25="-0.244240" step26="0.063326" step27="0.142369" step28="0.203528" step29="0.214331" step30="-0.667531" step31="0.326090" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="32" step0="-0.098422" step1="-0.295755" step2="-0.885922" step3="0.215369" step4="0.566637" step5="0.605213" step6="0.039766" step7="-0.396100" step8="0.751946" step9="0.453352" step10="0.911802" step11="0.851436" step12="0.078707" step13="-0.715323" step14="-0.075838" step15="-0.529344" step16="0.724479" step17="-0.580798" step18="0.559313" step19="0.687307" step20="0.993591" step21="0.999390" step22="0.222999" step23="-0.215125" step24="-0.467574" step25="-0.405438" step26="0.680288" step27="-0.952513" step28="-0.248268" step29="-0.814753" step30="0.354411" step31="-0.887570" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="3.550000" />
            <modulation identifier="2" src="stepLFO2" dest="f2p1" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Comb &amp; Phase Mod (random)" key_root="47" key_low="47" key_high="47" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-8.550000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="COMB1" p0="-0.310000" p1="0.700000" p2="0.160000" p3="-0.790000" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="PMOD" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="13" keytrigger="no" repeat="32" step0="-0.997497" step1="0.127171" step2="-0.613392" step3="0.617481" step4="0.170019" step5="-0.040254" step6="-0.299417" step7="0.791925" step8="0.645680" step9="0.493210" step10="-0.651784" step11="0.717887" step12="0.421003" step13="0.027070" step14="-0.392010" step15="-0.970031" step16="-0.817194" step17="-0.271096" step18="-0.705374" step19="-0.668203" step20="0.977050" step21="-0.108615" step22="-0.761834" step23="-0.990661" step24="-0.982177" step25="-0.244240" step26="0.063326" step27="0.142369" step28="0.203528" step29="0.214331" step30="-0.667531" step31="0.326090" />
            <lfo identifier="1" rate="0.000000" smooth="1.000000" syncmode="10" keytrigger="no" repeat="32" step0="-0.098422" step1="-0.295755" step2="-0.885922" step3="0.215369" step4="0.566637" step5="0.605213" step6="0.039766" step7="-0.396100" step8="0.751946" step9="0.453352" step10="0.911802" step11="0.851436" step12="0.078707" step13="-0.715323" step14="-0.075838" step15="-0.529344" step16="0.724479" step17="-0.580798" step18="0.559313" step19="0.687307" step20="0.993591" step21="0.999390" step22="0.222999" step23="-0.215125" step24="-0.467574" step25="-0.405438" step26="0.680288" step27="-0.952513" step28="-0.248268" step29="-0.814753" step30="0.354411" step31="-0.887570" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="3.550000" />
            <modulation identifier="2" src="stepLFO2" dest="f2p1" amount="20.000002" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="MicroGate" key_root="48" key_low="48" key_high="48" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-8.550000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="MGATE" p0="-3.600000" p1="0.530000" p2="-10.050001" p3="-8.399998" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="NONE" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="13" keytrigger="no" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="1.000000" syncmode="10" keytrigger="no" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="none" amount="3.550000" />
            <modulation identifier="2" src="stepLFO2" dest="f2p1" amount="20.000002" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="MicroGate &amp; Slewer" key_root="49" key_low="49" key_high="49" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="1.800000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="MGATE" p0="-3.600000" p1="0.530000" p2="-10.050001" p3="-8.399998" p4="22.300001" p5="4.250000" />
            <filter identifier="1" type="SLEW" p0="31.500000" p1="1.080000" p2="0.190000" p3="-0.890000" p4="11.200001" p5="2.840000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="1.000000" syncmode="13" keytrigger="no" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="1" rate="0.000000" smooth="1.000000" syncmode="4" keytrigger="no" repeat="32" step1="0.195085" step2="0.382673" step3="0.555556" step4="0.707090" step5="0.831454" step6="0.923866" step7="0.980777" step8="1.000000" step9="0.980795" step10="0.923902" step11="0.831505" step12="0.707156" step13="0.555633" step14="0.382758" step15="0.195176" step16="0.000093" step17="-0.194994" step18="-0.382587" step19="-0.555479" step20="-0.707025" step21="-0.831402" step22="-0.923831" step23="-0.980759" step24="-1.000000" step25="-0.980814" step26="-0.923937" step27="-0.831556" step28="-0.707221" step29="-0.555710" step30="-0.382844" step31="-0.195266" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="2" src="stepLFO2" dest="f2p2" amount="8.480000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Record Stop" key_root="50" key_low="50" key_high="50" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-1.500000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="LP4Msat" p0="5.500000" p2="4.000000" />
            <filter identifier="1" type="NONE" p1="0.500000" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="10" keytrigger="yes" repeat="16" step1="0.062500" step2="0.125000" step3="0.187500" step4="0.250000" step5="0.312500" step6="0.375000" step7="0.437500" step8="0.500000" step9="0.562500" step10="0.625000" step11="0.687500" step12="0.750000" step13="0.812500" step14="0.875000" step15="0.937500" step16="-1.000000" step17="-0.937500" step18="-0.875000" step19="-0.812500" step20="-0.750000" step21="-0.687500" step22="-0.625000" step23="-0.562500" step24="-0.500000" step25="-0.437500" step26="-0.375000" step27="-0.312500" step28="-0.250000" step29="-0.187500" step30="-0.125000" step31="-0.062500" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p1" amount="6.079999" />
            <modulation identifier="2" src="stepLFO1" dest="rate" amount="-1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Timestretch 1" key_root="51" key_low="51" key_high="51" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="12" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="6" loop_end="522" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="pos_2bars" dest="loopstart" amount="1.000500" />
            <modulation identifier="2" src="time60" dest="looplength" amount="0.510000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Timestretch 2" key_root="52" key_low="52" key_high="52" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="6" loop_end="2218" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_4bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="pos_4bars" dest="loopstart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Freqshift + Dist" key_root="53" key_low="53" key_high="53" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="FREQSHIFT" p0="-2.800000" />
            <filter identifier="1" type="DIST1" p0="14.250001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Snappy BPF" key_root="54" key_low="54" key_high="54" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-5.700000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="-7.300000" ef_release="-0.500000" lag0="-0.600000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="BP2B" p0="1.190000" p1="0.210000" p2="-4.000000" />
            <filter identifier="1" type="LMTR" p0="-27.900000" p1="-8.900000" p2="-2.800000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-2.920000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="envfollow" dest="f1p1" amount="-1.020000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Gate + Comb" key_root="55" key_low="55" key_high="55" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="GATE" p0="-7.100000" p1="-12.000000" p2="-96.000000" />
            <filter identifier="1" type="COMB1" p0="0.170000" p1="-0.770000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Scratch-ish" key_root="56" key_low="56" key_high="56" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="6" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-5.100000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="HP2A" p0="-1.130000" />
            <filter identifier="1" type="LMTR" p0="-10.050001" p1="-7.300000" p2="-3.200000" />
            <envelope identifier="0" attack="-6.200000" attack_shape="2.260000" hold="-2.050000" decay="-5.300000" decay_shape="1.000000" sustain="0.000000" release="-5.100000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.900000" attack_shape="7.039999" hold="-10.000000" decay="-2.995000" decay_shape="-0.760000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="EG2" dest="rate" amount="-1.100000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="morphEQ combs" key_root="57" key_low="57" key_high="57" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-6.150000" aux_level="0.000000" pan="0.000000" prefilter_gain="-0.600001" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="MORPHEQ" p0="4" pb0="0" p1="11" pb1="0" p3="2.510000" p4="-12.150002" p5="-0.005500" />
            <filter identifier="1" type="LMTR" p0="-12.599999" p1="-5.800000" p2="-1.300000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-1.120000" smooth="1.000000" syncmode="0" keytrigger="no" repeat="32" step0="-0.139244" step1="0.052205" step2="-0.033113" step3="0.247951" step4="0.253591" step5="0.318229" step6="0.195923" step7="0.399384" step8="0.325199" step9="0.201477" step10="0.024433" step11="-0.039216" step12="-0.346233" step13="-0.484652" step14="-0.631141" step15="-0.686380" step16="-0.296963" step17="-0.155248" step18="-0.253395" step19="-0.310453" step20="-0.373248" step21="-0.617505" step22="-0.583117" step23="-0.402277" step24="-0.163439" step25="0.075863" step26="-0.008795" step27="0.043757" step28="-0.184216" step29="-0.199487" step30="-0.365032" step31="-0.108029" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="time" dest="f1p4" amount="-1.340000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Dissolving" key_root="58" key_low="58" key_high="58" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="GATE" p0="-3.900000" p1="-9.900001" p2="-96.000000" />
            <filter identifier="1" type="COMB1" p0="-3.960000" p1="-0.010000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="3.300000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="EG2" dest="f1p2" amount="9.000001" />
            <modulation identifier="2" src="EG2" dest="f1p1" amount="-5.630000" />
            <modulation identifier="3" src="EG2" dest="f2p2" amount="-0.700000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Freqshift" key_root="59" key_low="59" key_high="59" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-16.200001" aux_level="0.000000" pan="0.000000" prefilter_gain="14.850000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="FREQSHIFT" p0="0.050000" />
            <filter identifier="1" type="LMTR" p0="-15.600000" p1="-8.000000" p2="-3.100000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Formant" key_root="60" key_low="60" key_high="60" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="6" loop_end="156656" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.900000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="MORPHEQ" p0="9" pb0="0" p1="7" pb1="0" p3="0.050000" p4="-10.200001" p5="0.190000" />
            <filter identifier="1" type="LMTR" p0="-12.600000" p1="-9.500000" p2="-3.000000" p3="-8.700001" p4="2.350000" p5="2.880000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="8" step0="-1.000000" step1="-1.000000" step2="-1.000000" step3="1.000000" step4="1.000000" step5="1.000000" step6="1.000000" step7="1.000000" step8="0.922078" step9="1.000000" step10="1.000000" step11="1.000000" step12="1.000000" step13="1.000000" step14="1.000000" step15="1.000000" step16="0.012987" step17="0.012987" step18="-0.012987" step19="-0.012987" step20="-0.012987" step21="0.012987" step22="0.038961" step23="0.064935" step24="0.090909" step25="0.064935" step26="0.090909" step27="0.090909" step28="0.090909" step29="0.116883" step30="0.090909" step31="0.064935" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p3" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Random arp" key_root="61" key_low="61" key_high="61" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="SLEW" p0="25.500000" p1="-1.890000" p2="0.080000" p3="0.740000" p4="20.049999" p5="4.230000" />
            <filter identifier="1" type="NONE" p0="12.900001" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="4" keytrigger="no" repeat="32" step0="-0.982421" step1="0.837581" step2="-0.448225" step3="-0.454207" step4="0.175817" step5="0.382366" step6="0.675222" step7="0.452986" step8="-0.030122" step9="-0.589282" step10="0.487472" step11="-0.063082" step12="-0.084078" step13="0.898312" step14="0.488876" step15="-0.783441" step16="0.198096" step17="-0.229530" step18="0.470016" step19="0.217933" step20="0.144810" step21="-0.277322" step22="-0.696890" step23="-0.549791" step24="-0.149693" step25="0.605762" step26="0.034211" step27="0.979980" step28="0.503098" step29="-0.308878" step30="-0.662038" step31="0.314615" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p2" amount="1.280000" />
            <modulation identifier="2" src="time" dest="f1p2" amount="0.380000" />
            <modulation identifier="3" src="time" dest="f1p1" amount="0.900000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Freqshift + Gate" key_root="62" key_low="62" key_high="62" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-8.400001" aux_level="0.000000" pan="0.000000" prefilter_gain="19.650000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="FREQSHIFT" p0="-4.900000" p1="5.700000" />
            <filter identifier="1" type="GATE" p0="-5.000000" p1="-9.750000" p2="-96.000000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="BPF + limiter sequenced" key_root="63" key_low="63" key_high="63" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-10.349999" aux_level="0.000000" pan="0.000000" prefilter_gain="1.500000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="BP2AD" p0="-1.200000" p1="0.100000" p3="1.530000" p4="2.000000" p5="1.000000" />
            <filter identifier="1" type="LMTR" p0="-39.000000" p1="-7.800000" p2="-4.600000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="4" keytrigger="no" repeat="32" step0="1.000000" step1="0.766234" step2="0.324675" step3="0.246753" step11="0.038961" step12="0.168831" step13="0.246753" step14="0.298701" step15="0.350649" step16="0.454545" step17="0.402597" step18="0.272727" step19="0.168831" step20="0.090909" step21="0.064935" step28="0.142857" step29="0.350649" step30="0.766234" step31="1.000000" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f1p4" amount="1.750000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Gate + Freqshift" key_root="64" key_low="64" key_high="64" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-1.649998" aux_level="0.000000" pan="0.000000" prefilter_gain="2.850000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="-5.700000" ef_release="-2.800000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="GATE" p0="-4.099999" p1="-8.700000" p2="-96.000000" p3="4.260000" p4="0.790000" />
            <filter identifier="1" type="FREQSHIFT" p1="0.050000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="0.000000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.220000" syncmode="4" keytrigger="no" repeat="16" step0="-0.090909" step1="-0.090909" step2="-0.766234" step3="-0.402597" step4="0.584416" step5="-0.402597" step6="-0.766234" step7="-1.000000" step8="0.480519" step9="-1.000000" step10="-0.038961" step11="-0.194805" step12="0.636364" step13="0.454545" step14="-0.402597" step15="-0.740260" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f2p1" amount="2.670001" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Sequenced Comb" key_root="65" key_low="65" key_high="65" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-8.100000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="NONE" p0="-4.600000" p1="-6.750000" p2="-96.000000" />
            <filter identifier="1" type="COMB1" p0="-2.140000" p1="0.790000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.700000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="8" step1="0.688312" step3="0.246753" step4="-0.012987" step5="-0.454545" step6="-0.194805" step7="-0.272727" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f2p1" amount="1.340000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Sequenced Comb (gated)" key_root="66" key_low="66" key_high="66" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="-8.100000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="GATE" p0="-4.600000" p1="-6.750000" p2="-96.000000" />
            <filter identifier="1" type="COMB1" p0="-2.140000" p1="0.790000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.700000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="8" step1="0.688312" step3="0.246753" step4="-0.012987" step5="-0.454545" step6="-0.194805" step7="-0.272727" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f2p1" amount="1.340000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
        <zone name="Sequenced Bell (gated)" key_root="67" key_low="67" key_high="67" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="156800" loop_start="0" loop_end="156800" loop_crossfade_length="0" velsense="0.000000" keytrack="1.000000" amplitude="1.050000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\rozzer shortcircuit temposync demo samples\funky beat.wav">
            <filter identifier="0" type="GATE" p0="-9.000000" p1="-7.349999" p2="-96.000000" />
            <filter identifier="1" type="MORPHEQ" p0="3" pb0="0" p1="0" pb1="0" p3="0.505000" p4="6.600000" p5="-0.004000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.700000" decay_shape="1.000000" sustain="1.000000" release="-4.500000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="7" keytrigger="no" repeat="8" step1="0.688312" step3="0.246753" step4="-0.012987" step5="-0.454545" step6="-0.194805" step7="-0.272727" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="pos_2bars" dest="samplestart" amount="1.000000" />
            <modulation identifier="1" src="stepLFO1" dest="f2p4" amount="1.340000" />
            <slice identifier="0" samplepos="0" end="156800" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="0" poly_cap="256" />
</shortcircuit>
