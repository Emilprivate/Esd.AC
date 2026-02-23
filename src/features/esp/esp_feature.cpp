#include "features/esp/esp_feature.h"

#include "imgui.h"

namespace {
    constexpr float kFeetOffset = 4.5f;
    constexpr float kBoxThickness = 1.5f;
    constexpr float kSnaplineThickness = 1.0f;

    float AbsFloat(float value) {
        return (value >= 0.0f) ? value : -value;
    }

    void DrawLine(const ImVec2& from, const ImVec2& to, ImU32 color, float thickness) {
        ImGui::GetBackgroundDrawList()->AddLine(from, to, color, thickness);
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

    const ImU32 color = IM_COL32(255, 0, 0, 255);
    DrawBox(ImVec2(screenHeadPt.x, screenHeadPt.y), ImVec2(screenFeetPt.x, screenFeetPt.y), color, kBoxThickness);

    if (drawSnaplines) {
        DrawLine(
            ImVec2(screenWidth / 2.0f, static_cast<float>(screenHeight)),
            ImVec2(screenFeetPt.x, screenFeetPt.y),
            color,
            kSnaplineThickness);
    }

    return true;
}