# The layers and folders that the files of each layer or folder may include from outside themselves.
# Checked by CheckIncludeDirections.cmake. A new layer, or a new folder in a layer with folder rules, needs a rule here.

set(LAYER_model "")
set(FOLDER_model "model/event;model/track;model/timeline;model/harmony")
set(FOLDER_model/event "")
set(FOLDER_model/controller "model/event")
set(FOLDER_model/track "model/event")
set(FOLDER_model/timeline "")
set(FOLDER_model/harmony "")
set(FOLDER_model/grid "model/timeline")
set(FOLDER_model/notation "model/harmony")

set(LAYER_edit "model")
set(FOLDER_edit/event "")
set(FOLDER_edit/controller "")
set(FOLDER_edit/track "")
set(FOLDER_edit/timeline "")
set(FOLDER_edit/harmony "")

set(LAYER_settings "")

set(LAYER_engine "model;edit")
set(FOLDER_engine/transport "")
set(FOLDER_engine/metronome "")
set(FOLDER_engine/recording "engine/transport")
set(FOLDER_engine/playback "engine/recording")
set(FOLDER_engine/chase "engine/playback")
set(FOLDER_engine/thru "engine/playback")

set(LAYER_undo "model;edit;engine")
set(FOLDER_undo "")
set(FOLDER_undo/event "undo")
set(FOLDER_undo/track "undo")

set(LAYER_io "model")
set(FOLDER_io/smf "")
set(FOLDER_io/project "")

set(LAYER_document "model;edit;settings;undo;io")

set(LAYER_audio "model;engine")
set(LAYER_device "model;engine")
set(LAYER_plugin "model;engine")

set(LAYER_session "model;edit;engine;undo;device")

set(LAYER_ui "model;edit;settings;engine;undo;io;document;audio;device;plugin;session")

set(LAYER_app "model;edit;settings;engine;undo;io;document;audio;device;plugin;session;ui")
