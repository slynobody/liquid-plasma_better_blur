/*
    Better Blur DX inter-effect mesh API v1.
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QVariant>
#include <QVariantMap>

namespace BetterBlurDxApi
{
inline constexpr int Version = 1;

// Better Wobbly publishes RequestRole during prePaintWindow(). Better Blur DX
// owns blur generation and the final mesh composite. ResultRole remains visible
// only during the nested drawWindow() chain so the requester can choose whether
// its standalone fallback is needed.
inline constexpr int RequestRole = 0x42775742; // BWwB
inline constexpr int ResultRole = 0x42775743;  // BWwC

struct MeshVertex
{
    // Final deformed position in device pixels.
    float x = 0.0f;
    float y = 0.0f;

    // Source position in Better Blur DX cache pixels. The cache remains owned
    // and sampled by Better Blur DX; Wobbly supplies geometry only.
    float cacheX = 0.0f;
    float cacheY = 0.0f;
};

struct MeshBuildInput
{
    quintptr effectWindow = 0;
    quintptr windowPaintData = 0;
    double renderScale = 1.0;
    double cacheOriginX = 0.0;
    double cacheOriginY = 0.0;
    int textureWidth = 0;
    int textureHeight = 0;
};

struct MeshView
{
    const MeshVertex *vertices = nullptr;
    int vertexCount = 0;
    // Additional opacity supplied by Wobbly. Better Blur DX multiplies this by
    // its own effective window opacity.
    float opacity = 1.0f;
};

using BuildMeshCallback = bool (*)(quintptr context,
                                   const MeshBuildInput *input,
                                   MeshView *output);

struct MeshProvider
{
    quint32 apiVersion = Version;
    quint32 structSize = sizeof(MeshProvider);
    quintptr context = 0;
    BuildMeshCallback buildMesh = nullptr;
};

enum class ProviderResult : int {
    None = 0,
    Rendered = 1,
    Declined = 2,
    // The provider intentionally excluded this window according to its own
    // blur policy (for example Blur only matching / Blur all except matching).
    // Consumers must not replace this decision with a fallback blur.
    DeclinedByPolicy = 3,
};

inline QVariant makeRequest(const MeshProvider *provider,
                            bool suppressDefaultComposite = true,
                            bool requireFreshCache = true)
{
    return QVariantMap{
        {QStringLiteral("apiVersion"), Version},
        {QStringLiteral("provider"), qulonglong(reinterpret_cast<quintptr>(provider))},
        {QStringLiteral("suppressDefaultComposite"), suppressDefaultComposite},
        {QStringLiteral("requireFreshCache"), requireFreshCache},
    };
}

inline bool isRequest(const QVariant &request)
{
    return request.toMap().value(QStringLiteral("apiVersion")).toInt() == Version;
}

inline bool suppressDefaultComposite(const QVariant &request)
{
    const QVariantMap map = request.toMap();
    return map.value(QStringLiteral("apiVersion")).toInt() == Version
        && map.value(QStringLiteral("suppressDefaultComposite")).toBool();
}

inline bool requireFreshCache(const QVariant &request)
{
    const QVariantMap map = request.toMap();
    return map.value(QStringLiteral("apiVersion")).toInt() == Version
        && map.value(QStringLiteral("requireFreshCache")).toBool();
}

inline const MeshProvider *decodeProvider(const QVariant &request)
{
    const QVariantMap map = request.toMap();
    if (map.value(QStringLiteral("apiVersion")).toInt() != Version) {
        return nullptr;
    }

    const quintptr address = quintptr(map.value(QStringLiteral("provider")).toULongLong());
    if (!address) {
        return nullptr;
    }

    const auto *provider = reinterpret_cast<const MeshProvider *>(address);
    if (provider->apiVersion != Version
        || provider->structSize < sizeof(MeshProvider)
        || !provider->buildMesh) {
        return nullptr;
    }
    return provider;
}

inline QVariant encodeProviderResult(ProviderResult result)
{
    return QVariantMap{
        {QStringLiteral("apiVersion"), Version},
        {QStringLiteral("result"), int(result)},
    };
}

inline ProviderResult decodeProviderResult(const QVariant &value)
{
    const QVariantMap map = value.toMap();
    if (map.value(QStringLiteral("apiVersion")).toInt() != Version) {
        return ProviderResult::None;
    }
    return ProviderResult(map.value(QStringLiteral("result")).toInt());
}

inline bool providerRendered(const QVariant &value)
{
    return decodeProviderResult(value) == ProviderResult::Rendered;
}

inline bool providerDeclinedByPolicy(const QVariant &value)
{
    return decodeProviderResult(value) == ProviderResult::DeclinedByPolicy;
}
}
