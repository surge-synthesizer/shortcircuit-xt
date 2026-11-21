<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<shortcircuit app="shortcircuit" version="1.0" revision="4">
    <group is_root_group="yes" />
    <group name="Kik" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="12" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" p1="-6.195860" p2="-2.400000" p3="0.700000" p4="0.150000" p5="-2.600000" p6="4.960000" />
        <effect identifier="1" type="NONE" p0="-7.050000" p1="-0.536965" p2="-12.000000" p3="-2.890000" p4="4.450000" p5="-1.000000" p6="4.000000" />
        <grouplfo rate="2.400000" smooth="0.000000" syncmode="0" repeat="16" />
        <modulation identifier="0" src="groupLFO" dest="none" amount="0.035000" />
        <modulation identifier="1" src="groupLFO" dest="none" amount="2.000000" />
        <zone name="Kik 1" key_root="48" key_low="48" key_high="48" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="RING" p0="-7.000000" p1="-0.000001" p2="4.000000" p3="3.599994" />
            <filter identifier="1" type="OD" p1="-1.830000" p2="18.000000" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="-5.645000" decay_shape="0.520000" sustain="0.000000" release="-5.645000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.221928" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="0.000000" syncmode="9" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-1.720000" smooth="0.000000" syncmode="13" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-0.994000" smooth="0.000000" syncmode="1" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="f1p1" amount="7.240000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Kik 2" key_root="50" key_low="50" key_high="50" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-24" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="12.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="RING" p0="-5.050000" p1="-0.000001" p2="4.000000" p3="3.599994" />
            <filter identifier="1" type="OD" p0="0.400000" p1="0.050000" p2="18.000000" p3="2.030000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="-5.645000" decay_shape="0.520000" sustain="0.000000" release="-5.645000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.221928" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="0.000000" syncmode="9" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-1.720000" smooth="0.000000" syncmode="13" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-0.994000" smooth="0.000000" syncmode="1" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="f1p1" amount="5.010000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Kik 3" key_root="52" key_low="52" key_high="52" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="5" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="12.000001" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="-5.050000" p1="-0.000001" p2="4.000000" p3="3.599994" />
            <filter identifier="1" type="OD" p1="0.090000" p2="9.000000" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="-5.645000" decay_shape="0.520000" sustain="0.000000" release="-5.645000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.221928" decay_shape="0.000000" sustain="0.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="0.000000" syncmode="9" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-1.720000" smooth="0.000000" syncmode="13" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-0.994000" smooth="0.000000" syncmode="1" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="16.660000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Kik 4" key_root="53" key_low="53" key_high="53" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-9" finetune="0.230000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="6.000001" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="-5.050000" p1="-0.000001" p2="4.000000" p3="3.599994" />
            <filter identifier="1" type="OD" p0="0.320000" p1="-1.810000" p2="12.000000" p3="5.500000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.460000" hold="-10.000000" decay="-3.200000" decay_shape="-2.020000" sustain="0.000000" release="-3.200000" release_shape="0.000000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-7.321928" decay_shape="-2.040000" sustain="0.000000" release="-7.320000" release_shape="0.000000" />
            <lfo identifier="0" rate="-3.200000" smooth="0.000000" syncmode="9" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="-1.720000" smooth="0.000000" syncmode="13" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="-0.994000" smooth="0.000000" syncmode="1" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="pitch" amount="23.959999" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <group name="Snare" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Snare 1" key_root="60" key_low="60" key_high="60" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-5" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="BP2A" p0="2.400000" p1="0.200000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.000000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 1" key_root="60" key_low="60" key_high="60" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="11" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="-1.960000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="5.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 2" key_root="62" key_low="62" key_high="62" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-13" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="BP2A" p0="-0.070000" p1="0.500000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.506353" decay_shape="-1.020000" sustain="0.000000" release="-3.506353" release_shape="-1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="f1p1" amount="1.500000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 2" key_root="62" key_low="62" key_high="62" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="25" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.020000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="14.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 3" key_root="64" key_low="64" key_high="64" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="25" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.020000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="14.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 3" key_root="64" key_low="64" key_high="64" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-5" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="BP2A" p0="2.400000" p1="0.200000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.000000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 4" key_root="65" key_low="65" key_high="65" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="16" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.020000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="14.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Snare 4" key_root="65" key_low="65" key_high="65" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-17" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-30.000000" keytrack="1.000000" amplitude="6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="BP2A" p0="5.160000" p1="0.500000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.506353" decay_shape="1.000000" sustain="0.000000" release="-4.506353" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <group name="Hi Hat" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Cl HH 1" key_root="72" key_low="72" key_high="72" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="PKA" p0="4.200000" p1="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.600000" decay_shape="6.960000" sustain="0.000000" release="-10.000000" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Op HH 1" key_root="73" key_low="73" key_high="73" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-9.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="PKA" p0="4.200000" p1="1.000000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-0.321928" decay_shape="7.000000" sustain="0.000000" release="-0.321928" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Cl HH 2" key_root="74" key_low="74" key_high="74" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-20" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="PKA" p0="3.250000" p1="0.640000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.600000" decay_shape="6.960000" sustain="0.000000" release="-10.000000" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Op HH 2" key_root="75" key_low="75" key_high="75" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-20" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-6.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="PKA" p0="3.250000" p1="0.640000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-0.736966" decay_shape="7.000000" sustain="0.000000" release="-0.736966" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Cl HH 3" key_root="77" key_low="77" key_high="77" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-10" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-3.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="HP2A" p0="3.200000" p1="0.120000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-3.600000" decay_shape="6.960000" sustain="0.000000" release="-10.000000" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Op HH 3" key_root="77" key_low="78" key_high="78" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="-10" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="86401" loop_start="0" loop_end="86401" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="-9.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\noise white.wav">
            <filter identifier="0" type="HP2A" p0="3.200000" p1="0.120000" />
            <filter identifier="1" type="NONE" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-0.689660" decay_shape="20.000000" sustain="0.000000" release="-0.689660" release_shape="1.040000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <group name="Percussion" key_low="0" key_high="127" lo_vel="0" hi_vel="127" channel="0" output="0" transpose="0" coarsetune="0" finetune="0.000000" amplitude="0.000000" balance="0.000000" group_mix="yes">
        <effect identifier="0" type="NONE" />
        <effect identifier="1" type="NONE" />
        <grouplfo rate="0.000000" smooth="0.000000" syncmode="0" repeat="16" />
        <zone name="Clave" key_root="48" key_low="48" key_high="48" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="59" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="1.750000" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-4.321928" decay_shape="20.000000" sustain="0.000000" release="-4.321928" release_shape="2.020000" />
            <envelope identifier="1" attack="-5.800000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="10.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="f2p1" amount="-5.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Tom Lo" key_root="50" key_low="50" key_high="50" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="14" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="1.750000" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="20.000000" sustain="0.000000" release="-2.000000" release_shape="2.020000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="10.000000" sustain="0.000000" release="-2.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="pitch" amount="5.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Tom Mid" key_root="52" key_low="52" key_high="52" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="20" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="1.750000" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="20.000000" sustain="0.000000" release="-2.000000" release_shape="2.020000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="10.000000" sustain="0.000000" release="-2.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="pitch" amount="5.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Tom Hi" key_root="53" key_low="53" key_high="53" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="28" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="43151" loop_start="0" loop_end="43151" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="1.750000" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.736965" decay_shape="20.000000" sustain="0.000000" release="-2.736965" release_shape="2.020000" />
            <envelope identifier="1" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="10.000000" sustain="0.000000" release="-2.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="EG2" dest="pitch" amount="5.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Sim Lo" key_root="55" key_low="55" key_high="55" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="0" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="42478" loop_start="0" loop_end="42478" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\pulse c2.wav">
            <filter identifier="0" type="LP4Msat" p0="2.180000" p1="0.300000" p2="4.500000" p3="9.000001" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="5.000000" sustain="0.000000" release="-2.000000" release_shape="2.020000" />
            <envelope identifier="1" attack="-5.800000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="10.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="10.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Sim Hi" key_root="57" key_low="57" key_high="57" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="20" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="42478" loop_start="0" loop_end="42478" loop_crossfade_length="0" velsense="-49.950001" keytrack="1.000000" amplitude="0.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="1" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\pulse c2.wav">
            <filter identifier="0" type="LP4Msat" p0="2.180000" p1="0.300000" p2="4.500000" p3="9.000001" />
            <filter identifier="1" type="NONE" p0="4.100000" p1="0.280000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="5.000000" sustain="0.000000" release="-2.000000" release_shape="2.020000" />
            <envelope identifier="1" attack="-5.800000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="10.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="AEG" dest="pitch" amount="10.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
        <zone name="Sine C2.wav" key_root="59" key_low="59" key_high="59" lo_vel="0" hi_vel="127" channel="0" output="0" aux_output="-1" transpose="30" finetune="0.000000" pitchcorrect="0.000000" playmode="fwd_loop" sample_start="0" sample_end="42478" loop_start="0" loop_end="42478" loop_crossfade_length="0" velsense="-50.000000" keytrack="1.000000" amplitude="-9.000000" aux_level="0.000000" pan="0.000000" prefilter_gain="0.000000" PB_depth="2" mute_group="0" polymode="0" portamode="0" portamento="-10.000000" mute="no" mono_output="no" aux_mono_output="no" ef_attack="0.000000" ef_release="0.000000" lag0="0.000000" lag1="0.000000" filename="$relative\..\samples\sine c2.wav">
            <filter identifier="0" type="NONE" p0="6.000000" />
            <filter identifier="1" type="OD" p1="0.650000" p2="24.000000" p3="-2.080000" p4="0.800000" />
            <envelope identifier="0" attack="-10.000000" attack_shape="0.000000" hold="-10.000000" decay="-5.300000" decay_shape="0.980000" sustain="0.000000" release="-5.300000" release_shape="1.000000" />
            <envelope identifier="1" attack="-2.000000" attack_shape="0.000000" hold="-10.000000" decay="-2.000000" decay_shape="0.000000" sustain="1.000000" release="-5.000000" release_shape="0.000000" />
            <lfo identifier="0" rate="5.000000" smooth="0.000000" syncmode="0" keytrigger="yes" repeat="4" step0="1.000000" step1="-0.272727" step2="0.168831" step3="-0.662338" />
            <lfo identifier="1" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <lfo identifier="2" rate="0.000000" smooth="0.000000" syncmode="0" keytrigger="no" repeat="16" />
            <modulation identifier="0" src="stepLFO1" dest="pitch" amount="6.007498" />
            <modulation identifier="1" src="AEG" dest="f2p4" amount="8.000000" />
            <slice identifier="0" samplepos="0" env="0.000000" mute="no" />
        </zone>
    </group>
    <global headroom="6" poly_cap="256" />
</shortcircuit>
