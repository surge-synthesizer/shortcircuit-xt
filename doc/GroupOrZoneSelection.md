# Group and Zone Selection

The result of a conversation we had on discord.

Group selection and Zone selection are indeed totally separate.
User can switch between group and zone mode.
The two selections are both retained when switching between modes.
If you’re in group mode, selected groups have the more opaque
selection marker, zones have the more subtle one. Switch modes
and those flip so you can easily see which mode you’re in.

## In Zone mode:

Clicking on a zone in the tree selects it (and multiselect gestures work as expected on zones).

Clicking on a group is just shorthand for selecting all zones in the group.

Zone ADSR/LFO/Matrix/Processor/Output panes are visible/editable.
Trig Conditions and Group Settings disappear.

## In Group mode:

Clicking on a group in the tree selects it. (and again multiselect etc).

Clicking on a zone changes leader zone. This only influences what you see in the mapping area. Nothing else about zone
selection can change while in Group mode.

Group ADSR/LFO/Matrix/Processor/Output panes are visible/editable.
Trig Conditions and Group Settings also become visible/editable)

The wireframe has been updated with to reflect some of these ideas so check that too. 🙂

A couple of not directly related things. We worked a little on the color choices too. And are leaning towards blue
signaling modulation always. It was used for some other things in the wireframe but we think it’ll be good to change
that. Group vs. Zone belonging can be communicated other ways than blue vs. yellow.
