#include "features/esp/esp_feature.h"

#include "imgui.h"


namespace {
    constexpr float kFeetOffset = 4.5f;
    constexpr float kSnaplineThickness = 1.0f;
    constexpr float kBarWidth = 4.0f;
    constexpr float kBarPadding = 2.0f;

    float AbsFloat(float value) {
        return (value >= 0.0f) ? value : -value;
    }

    void DrawLine(const ImVec2& from, const ImVec2& to, ImU32 color, float thickness) {
        ImGui::GetBackgroundDrawList()->AddLine(from, to, color, thickness);
    }

    void DrawFilledRect(const ImVec2& from, const ImVec2& to, ImU32 color) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(from, to, color);
    }

    void DrawBox(const ImVec2& top, const ImVec2& bottom, ImU32 color, float thickness) {
        const float height = bottom.y - top.y;
        if (height < 1.0f) {
            return;
        }

        const float width = height / 2.0f;
        const float centerX = (top.x + bottom.x) * 0.5f;

        const ImVec2 topLeft(centerX - width / 2.0f, top.y);
        const ImVec2 bottomRight(centerX + width / 2.0f, bottom.y);

        ImGui::GetBackgroundDrawList()->AddRect(topLeft, bottomRight, color, 0.0f, 0, thickness);
    }


    bool ProjectW2SLayout(
        const Vector3& worldPos,
        Vector3& screenOut,
        const float* matrix,
        int screenWidth,
        int screenHeight,
        bool rowMajor) {

        Vector4 clipCoords{};
        if (rowMajor) {
            clipCoords.x = worldPos.x * matrix[0] + worldPos.y * matrix[1] + worldPos.z * matrix[2] + matrix[3];
            clipCoords.y = worldPos.x * matrix[4] + worldPos.y * matrix[5] + worldPos.z * matrix[6] + matrix[7];
            clipCoords.z = worldPos.x * matrix[8] + worldPos.y * matrix[9] + worldPos.z * matrix[10] + matrix[11];
            clipCoords.w = worldPos.x * matrix[12] + worldPos.y * matrix[13] + worldPos.z * matrix[14] + matrix[15];
        } else {
            clipCoords.x = worldPos.x * matrix[0] + worldPos.y * matrix[4] + worldPos.z * matrix[8] + matrix[12];
            clipCoords.y = worldPos.x * matrix[1] + worldPos.y * matrix[5] + worldPos.z * matrix[9] + matrix[13];
            clipCoords.z = worldPos.x * matrix[2] + worldPos.y * matrix[6] + worldPos.z * matrix[10] + matrix[14];
            clipCoords.w = worldPos.x * matrix[3] + worldPos.y * matrix[7] + worldPos.z * matrix[11] + matrix[15];
        }

        if (clipCoords.w <= 0.05f) {
            return false;
        }

        Vector3 ndc{};
        ndc.x = clipCoords.x / clipCoords.w;
        ndc.y = clipCoords.y / clipCoords.w;
        ndc.z = clipCoords.z / clipCoords.w;

        if (ndc.z < -0.1f || ndc.z > 1.5f) {
            return false;
        }

        if (AbsFloat(ndc.x) > 2.5f || AbsFloat(ndc.y) > 2.5f) {
            return false;
        }

        const float halfWidth = static_cast<float>(screenWidth) * 0.5f;
        const float halfHeight = static_cast<float>(screenHeight) * 0.5f;

        screenOut.x = (halfWidth + ndc.x) + (halfWidth * ndc.x);
        screenOut.y = (halfHeight + ndc.y) - (halfHeight * ndc.y) + 5.0f;
        screenOut.z = ndc.z;

        if (screenOut.x < -300.0f || screenOut.x > (screenWidth + 300.0f) ||
            screenOut.y < -300.0f || screenOut.y > (screenHeight + 300.0f)) {
            return false;
        }

        return true;
    }

    bool ProjectEntity(
        const Vector3& worldHead,
        const Vector3& worldFeet,
        Vector3& screenHead,
        Vector3& screenFeet,
        const float* matrix,
        int screenWidth,
        int screenHeight) {

        auto projectPair = [&](bool rowMajor, Vector3& outHead, Vector3& outFeet) -> bool {
            if (!ProjectW2SLayout(worldHead, outHead, matrix, screenWidth, screenHeight, rowMajor)) {
                return false;
            }
            if (!ProjectW2SLayout(worldFeet, outFeet, matrix, screenWidth, screenHeight, rowMajor)) {
                return false;
            }

            const float boxHeight = outFeet.y - outHead.y;
            if (boxHeight < 4.0f || boxHeight > (screenHeight * 0.95f)) {
                return false;
            }

            const float horizontalDrift = AbsFloat(outHead.x - outFeet.x);
            if (horizontalDrift > (boxHeight * 0.6f)) {
                return false;
            }

            return true;
        };

        Vector3 colHead{}, colFeet{};
        Vector3 rowHead{}, rowFeet{};
        const bool colValid = projectPair(false, colHead, colFeet);
        const bool rowValid = projectPair(true, rowHead, rowFeet);

        if (!colValid && !rowValid) {
            return false;
        }

        if (colValid && rowValid) {
            const float colDrift = AbsFloat(colHead.x - colFeet.x);
            const float rowDrift = AbsFloat(rowHead.x - rowFeet.x);
            if (colDrift <= rowDrift) {
                screenHead = colHead;
                screenFeet = colFeet;
            } else {
                screenHead = rowHead;
                screenFeet = rowFeet;
            }
            return true;
        }

        if (colValid) {
            screenHead = colHead;
            screenFeet = colFeet;
            return true;
        }

        screenHead = rowHead;
        screenFeet = rowFeet;
        return true;
    }
}

bool EspFeature::TryDrawEntityEsp(
    const Player* entity,
    const float* viewMatrix,
    int screenWidth,
    int screenHeight,
    bool isTeammate,
    bool hasTeamData,
    int entityHealth,
    int entityArmor,
    const std::string& entityName,
    float entityDistance,
    bool showName,
    bool showHealth,
    bool showArmor,
    bool showDistance,
    bool showHealthValue,
    bool showArmorValue,
    bool useTeamColors,
    int infoPosition,
    bool boxFilled,
    float boxThickness,
    float boxFillAlpha,
    bool drawSnaplines,
    int& entitiesW2S) {

    if (!entity || !viewMatrix || screenWidth <= 0 || screenHeight <= 0) {
        return false;
    }

    const Vector3 entityHead = entity->GetHeadPos();
    const Vector3 entityFeet = {entityHead.x, entityHead.y, entityHead.z - kFeetOffset};

    Vector3 screenHeadPt{};
    Vector3 screenFeetPt{};
    if (!ProjectEntity(entityHead, entityFeet, screenHeadPt, screenFeetPt, viewMatrix, screenWidth, screenHeight)) {
        return false;
    }

    const float boxHeight = screenFeetPt.y - screenHeadPt.y;
    if (boxHeight <= 2.0f || boxHeight >= screenHeight * 0.9f) {
        return false;
    }

    entitiesW2S++;

    ImU32 color = IM_COL32(255, 0, 0, 255);
    if (useTeamColors && hasTeamData) {
        color = isTeammate ? IM_COL32(80, 200, 120, 255) : IM_COL32(220, 80, 80, 255);
    }
    const float boxWidth = boxHeight / 2.0f;
    const float boxLeft = screenHeadPt.x - boxWidth / 2.0f;
    const float boxRight = screenHeadPt.x + boxWidth / 2.0f;

    if (boxFilled && boxFillAlpha > 0.0f) {
        const ImU32 fillColor = IM_COL32(20, 20, 20, static_cast<int>(boxFillAlpha * 255.0f));
        DrawFilledRect(ImVec2(boxLeft, screenHeadPt.y), ImVec2(boxRight, screenFeetPt.y), fillColor);
    }

    DrawBox(ImVec2(screenHeadPt.x, screenHeadPt.y), ImVec2(screenFeetPt.x, screenFeetPt.y), color, boxThickness);

    if (showHealth) {
        const float health = static_cast<float>(entityHealth);
        const float healthRatio = (health > 0.0f) ? (health / 100.0f) : 0.0f;
        const float clamped = (healthRatio > 1.0f) ? 1.0f : (healthRatio < 0.0f ? 0.0f : healthRatio);

        const ImVec2 barTop(boxLeft - kBarPadding - kBarWidth, screenHeadPt.y);
        const ImVec2 barBottom(boxLeft - kBarPadding, screenFeetPt.y);
        DrawFilledRect(barTop, barBottom, IM_COL32(20, 20, 20, 200));

        const float filledHeight = boxHeight * clamped;
        const ImVec2 filledTop(boxLeft - kBarPadding - kBarWidth, screenFeetPt.y - filledHeight);
        const ImVec2 filledBottom(boxLeft - kBarPadding, screenFeetPt.y);
        DrawFilledRect(filledTop, filledBottom, IM_COL32(80, 220, 120, 255));
    }

    if (showArmor) {
        const float armor = static_cast<float>(entityArmor);
        const float armorRatio = (armor > 0.0f) ? (armor / 100.0f) : 0.0f;
        const float clamped = (armorRatio > 1.0f) ? 1.0f : (armorRatio < 0.0f ? 0.0f : armorRatio);

        const ImVec2 barTop(boxRight + kBarPadding, screenHeadPt.y);
        const ImVec2 barBottom(boxRight + kBarPadding + kBarWidth, screenFeetPt.y);
        DrawFilledRect(barTop, barBottom, IM_COL32(20, 20, 20, 200));

        const float filledHeight = boxHeight * clamped;
        const ImVec2 filledTop(boxRight + kBarPadding, screenFeetPt.y - filledHeight);
        const ImVec2 filledBottom(boxRight + kBarPadding + kBarWidth, screenFeetPt.y);
        DrawFilledRect(filledTop, filledBottom, IM_COL32(80, 140, 220, 255));
    }

    if (showName || showDistance || showHealthValue || showArmorValue) {
        ImVec2 textPos{};
        const float lineHeight = ImGui::GetFontSize() + 2.0f;
        const float startXCenter = screenHeadPt.x;

        switch (infoPosition) {
            case 1: {
                textPos = ImVec2(boxRight + kBarPadding + kBarWidth + 6.0f, screenHeadPt.y);
                break;
            }
            case 2: {
                textPos = ImVec2(startXCenter, screenFeetPt.y + 4.0f);
                break;
            }
            case 0:
            default: {
                textPos = ImVec2(startXCenter, screenHeadPt.y - 16.0f);
                break;
            }
        }

        auto drawLine = [&](const std::string& text) {
            if (text.empty()) {
                return;
            }

            ImVec2 pos = textPos;
            if (infoPosition != 1) {
                const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
                pos.x -= textSize.x * 0.5f;
            }

            ImGui::GetBackgroundDrawList()->AddText(pos, IM_COL32(240, 240, 240, 255), text.c_str());
            textPos.y += lineHeight;
        };

        if (showName && !entityName.empty()) {
            drawLine(entityName);
        }

        if (showDistance) {
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "%.1fm", entityDistance);
            drawLine(buffer);
        }

        if (showHealthValue) {
            char buffer[24]{};
            std::snprintf(buffer, sizeof(buffer), "HP: %d", entityHealth);
            drawLine(buffer);
        }

        if (showArmorValue) {
            char buffer[24]{};
            std::snprintf(buffer, sizeof(buffer), "AR: %d", entityArmor);
            drawLine(buffer);
        }
    }

    if (drawSnaplines) {
        DrawLine(
            ImVec2(screenWidth / 2.0f, static_cast<float>(screenHeight)),
            ImVec2(screenFeetPt.x, screenFeetPt.y),
            color,
            kSnaplineThickness);
    }

    return true;
}