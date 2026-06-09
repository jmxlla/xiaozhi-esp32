protocol_->OnIncomingJson([this, display](const cJSON* root) {

    auto type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) return;

    // ---------------- TTS ----------------
    if (strcmp(type->valuestring, "tts") == 0) {

        auto state = cJSON_GetObjectItem(root, "state");

        if (cJSON_IsString(state) && strcmp(state->valuestring, "start") == 0) {
            Schedule([this]() {
                aborted_ = false;
                SetDeviceState(kDeviceStateSpeaking);
            });

        } else if (cJSON_IsString(state) && strcmp(state->valuestring, "stop") == 0) {
            Schedule([this]() {
                if (GetDeviceState() == kDeviceStateSpeaking) {
                    if (listening_mode_ == kListeningModeManualStop) {
                        SetDeviceState(kDeviceStateIdle);
                    } else {
                        SetDeviceState(kDeviceStateListening);
                    }
                }
            });

        } else if (cJSON_IsString(state) && strcmp(state->valuestring, "sentence_start") == 0) {
            auto text = cJSON_GetObjectItem(root, "text");
            if (cJSON_IsString(text)) {
                Schedule([display, msg = std::string(text->valuestring)]() {
                    display->SetChatMessage("assistant", msg.c_str());
                });
            }
        }

    // ---------------- STT ----------------
    } else if (strcmp(type->valuestring, "stt") == 0) {

        auto text = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(text)) {
            Schedule([display, msg = std::string(text->valuestring)]() {
                display->SetChatMessage("user", msg.c_str());
            });
        }

    // ---------------- LLM ----------------
    } else if (strcmp(type->valuestring, "llm") == 0) {

        auto emotion = cJSON_GetObjectItem(root, "emotion");
        if (cJSON_IsString(emotion)) {
            Schedule([display, emo = std::string(emotion->valuestring)]() {
                display->SetEmotion(emo.c_str());
            });
        }

    // ---------------- MCP ----------------
    } else if (strcmp(type->valuestring, "mcp") == 0) {

        auto payload = cJSON_GetObjectItem(root, "payload");
        if (cJSON_IsObject(payload)) {
            McpServer::GetInstance().ParseMessage(payload);
        }

    // ---------------- SYSTEM ----------------
    } else if (strcmp(type->valuestring, "system") == 0) {

        auto command = cJSON_GetObjectItem(root, "command");
        if (cJSON_IsString(command)) {

            if (strcmp(command->valuestring, "reboot") == 0) {
                Schedule([this]() { Reboot(); });
            }
        }

    // ---------------- ALERT ----------------
    } else if (strcmp(type->valuestring, "alert") == 0) {

        auto status = cJSON_GetObjectItem(root, "status");
        auto message = cJSON_GetObjectItem(root, "message");
        auto emotion = cJSON_GetObjectItem(root, "emotion");

        if (cJSON_IsString(status) && cJSON_IsString(message) && cJSON_IsString(emotion)) {
            Alert(status->valuestring, message->valuestring, emotion->valuestring,
                  Lang::Sounds::OGG_VIBRATION);
        }

    // ---------------- MUSIC (NEW) ----------------
    } else if (strcmp(type->valuestring, "music") == 0) {

        auto name = cJSON_GetObjectItem(root, "name");

        if (cJSON_IsString(name)) {
            std::string song = name->valuestring;

            ESP_LOGI(TAG, "Playing music: %s", song.c_str());

            Schedule([this, song]() {
                audio_service_.PlaySound(song);
            });
        }

    } else {
        ESP_LOGW(TAG, "Unknown message type: %s", type->valuestring);
    }

});
