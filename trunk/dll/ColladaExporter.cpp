#include "ColladaExporter.h"

#include "common/XmlNode.h"
#include "D3DHelpers.h"

#include <cassert>
#include <fstream>

struct VertexDeclarationInfo
{
    std::string semanticName_;
    size_t defaultSize_;

    VertexDeclarationInfo(std::string&& semanticName, size_t defaultSize)
        : semanticName_(std::move(semanticName))
        , defaultSize_(defaultSize)
    {
    }
};

std::map<BYTE, VertexDeclarationInfo> s_vertexDeclarationInfoMap;

struct VertexComponentData
{
    const uint8_t* data_;
    size_t size_;

    VertexComponentData(const uint8_t* data, size_t size)
        : data_(data)
        , size_(size)
    {
    }
};

typedef std::map<std::string, std::vector<VertexComponentData>> VertexDataMap;

template <typename T>
size_t WriteIndicies_(const T* indexData, size_t numIndicies, std::string& output, D3DPRIMITIVETYPE primitiveType)
{
    size_t trianglesWritten = 0;
    output.reserve(numIndicies * 2);
    if (primitiveType == D3DPT_TRIANGLELIST)
    {
        for (size_t i = 0; i < numIndicies; ++i)
        {
            output.append(std::to_string(indexData[i]));
            output.append(1, ' ');
        }
        trianglesWritten = numIndicies / 3;
    }
    else if (primitiveType == D3DPT_TRIANGLESTRIP)
    {
        // 0 1 2  1 3 2  2 3 4  3 5 4 ...
        assert(numIndicies >= 3);
        for (size_t i = 2; i < numIndicies; ++i)
        {
            T idx1 = indexData[i - 2];
            T idx2 = indexData[i - 1 + i % 2];
            T idx3 = indexData[i - i % 2];
            if ((idx1 == idx2) || (idx1 == idx3) || (idx2 == idx3))
            {
                // degenerate triangle
                continue;
            }
            output.append(std::to_string(idx1));
            output.append(1, ' ');
            output.append(std::to_string(idx2));
            output.append(1, ' ');
            output.append(std::to_string(idx3));
            output.append(1, ' ');
            ++trianglesWritten;
        }
    }
    else if (primitiveType == D3DPT_TRIANGLEFAN)
    {
        // 1 2 0  3 4 0  5 6 0  7 8 0 ...
        for (size_t i = 1; i < (numIndicies - 1) / 2; i += 2)
        {
            output.append(std::to_string(indexData[i]));
            output.append(1, ' ');
            output.append(std::to_string(indexData[i + 1]));
            output.append(1, ' ');
            output.append(std::to_string(indexData[0]));
            output.append(1, ' ');
            ++trianglesWritten;
        }
    }
    else
    {
        LOG("Unsupported primitive type");
    }
    return trianglesWritten;
}

void AddComponent_(XmlNode& node, const uint8_t* data, size_t numVertices, const std::string& meshName, const std::string& componentName, size_t componentSize, size_t stride)
{
    const float* componentData = reinterpret_cast<const float*>(data);
    std::string componentDataValue;
    componentDataValue.reserve(numVertices * componentSize * 2);
    for (size_t i = 0; i < numVertices; ++i)
    {
        for (size_t j = 0; j < componentSize; ++j)
        {
            componentDataValue.append(std::to_string(componentData[j]));
            componentDataValue.append(1, ' ');
        }
        componentDataValue.append(1, ' ');
        componentData = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(componentData) + stride);
    }
    XmlNode& sourceNode = node.AddChild("source").AddParameter("id", meshName + "-" + componentName);
    sourceNode.AddChild("float_array")
        .AddParameter("id", meshName + "-" + componentName + "-array")
        .AddParameter("count", std::to_string(numVertices * componentSize))
        .SetValue(std::move(componentDataValue));
    XmlNode& accessorNode = sourceNode.AddChild("technique_common")
                                        .AddChild("accessor").AddParameter("source", "#" + meshName + "-" + componentName + "-array")
                                                            .AddParameter("count", std::to_string(numVertices))
                                                            .AddParameter("stride", std::to_string(componentSize));

    for (size_t i = 0; i < componentSize; ++i)
    {
        accessorNode.AddChild("param").AddParameter("name", std::string(1, char('X' + i))).AddParameter("type", "float");
    }
}

size_t GetComponentSize_(BYTE usage, BYTE type, size_t defaultSize)
{
    switch (type)
    {
    case D3DDECLTYPE_FLOAT1:
        return 1;
    case D3DDECLTYPE_FLOAT2:
        return 2;
    case D3DDECLTYPE_FLOAT3:
        return 3;
    case D3DDECLTYPE_UNUSED:
        return defaultSize;
    }

    return 0;
}


ColladaExporter::ColladaExporter(const std::string& saveFolder)
    : m_saveFolder(saveFolder)
    , m_imageCache(saveFolder)
    , m_primitiveNumber(0)
{
    static bool vertexDeclarationInit = false;
    if (!vertexDeclarationInit)
    {
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_POSITION, VertexDeclarationInfo(std::string("POSITION"), 3)));
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_NORMAL, VertexDeclarationInfo(std::string("NORMAL"), 3)));
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_TEXCOORD, VertexDeclarationInfo(std::string("TEXCOORD"), 2)));
        vertexDeclarationInit = true;
    }

    m_textures.reserve(MAX_TEXTURE_COUNT);
}

ColladaExporter::~ColladaExporter()
{

}

void ColladaExporter::StartScene()
{
    m_primitiveNumber = 0;
}

void ColladaExporter::SetTexture(size_t stage, size_t width, size_t height, D3DFORMAT format, void* pData, int pitch)
{
    m_textures.push_back(m_imageCache.Add(width, height, format, pData, pitch));
}

void ColladaExporter::AddPrimitive(D3DPRIMITIVETYPE primitiveType, 
                                    const uint8_t* vertices, 
                                    size_t numVertices, 
                                    D3DVERTEXELEMENT9* vertexDeclaration, 
                                    size_t stride, 
                                    const uint8_t* indicies, 
                                    size_t numIndicies, 
                                    D3DFORMAT indexFormat, 
                                    size_t primitiveCount)
{
    std::ofstream file;

    std::string filename = m_saveFolder + "\\primitive_" + std::to_string(m_primitiveNumber) + ".dae";
    file.open(filename, std::ios_base::out | std::ios_base::binary);
    file.precision(10);

    std::string meshName = "mesh_" + std::to_string(m_primitiveNumber);
    std::string primitiveName = "????";
    if (primitiveType == D3DPT_TRIANGLELIST
        || primitiveType == D3DPT_TRIANGLESTRIP
        || primitiveType == D3DPT_TRIANGLEFAN)
    {
        if (numIndicies < 3)
        {
            LOG("Invalid indicies count");
            return;
        }
        primitiveName = "triangles";
    }
    else
    {
        LOG("Unsupported primitive type %d", primitiveType);
        return;
    }

    XmlNode colladaRoot("COLLADA");
    colladaRoot.AddParameter("xmlns", "http://www.collada.org/2005/11/COLLADASchema")
               .AddParameter("version", "1.4.1");
    colladaRoot.AddChild("asset")
               .AddChild("unit").AddParameter("meter", "0.01").AddParameter("name", "centimeter");
    XmlNode& libraryImagesRoot = colladaRoot.AddChild("library_images");
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        if (m_textures[i].empty())
        {
            continue;
        }

        libraryImagesRoot.AddChild("image").AddParameter("id", "texture" + i)
            .AddChild("init_from").SetValue(m_textures[i]);
    }
    XmlNode& libraryMaterialsRoot = colladaRoot.AddChild("library_materials");
    libraryMaterialsRoot.AddChild("material").AddParameter("id", meshName + "-material")
        .AddChild("instance_effect").AddParameter("url", "#" + meshName + "-material-fx");

    XmlNode& libraryEffects = colladaRoot.AddChild("library_effects");
    XmlNode& profileCommon = libraryEffects.AddChild("effect").AddParameter("id", meshName + "-material-fx")
                                           .AddChild("profile_COMMON");

    profileCommon.AddChild("newparam").AddParameter("sid", "texture0-surface")
                .AddChild("surface").AddParameter("type", "2D")
                .AddChild("init_from").SetValue("texture0");
    profileCommon.AddChild("newparam").AddParameter("sid", "sampler2D_0")
        .AddChild("sampler2D")
        .AddChild("source").SetValue("texture0-surface");
    XmlNode& phong = profileCommon.AddChild("phong");
    phong.AddChild("emission")
        .AddChild("color").AddParameter("sid", "emission").SetValue("0.0 0.0 0.0 1.0");
    phong.AddChild("ambient")
        .AddChild("color").AddParameter("sid", "ambient").SetValue("0.5 0.5 0.5 1.0");
    phong.AddChild("diffuse")
        .AddChild("texture").AddParameter("texture", "sampler2D_0").AddParameter("texcoord", "CHANNEL0");
    phong.AddChild("specular")
        .AddChild("color").AddParameter("sid", "specular").SetValue("0.0 0.0 0.0 1.0");
    phong.AddChild("shininess")
        .AddChild("float").AddParameter("sid", "shininess").SetValue("2.0");
    phong.AddChild("reflective")
        .AddChild("color").AddParameter("sid", "reflective").SetValue("0.0 0.0 0.0 1.0");
    phong.AddChild("reflectivity")
        .AddChild("float").AddParameter("sid", "reflectivity").SetValue("1.0");
    phong.AddChild("transparent").AddParameter("opaque", "RGB_ZERO")
        .AddChild("color").AddParameter("sid", "transparent").SetValue("1.0 1.0 1.0 1.0");
    phong.AddChild("transparency")
        .AddChild("float").AddParameter("sid", "transparency").SetValue("0.0");

    XmlNode& libraryGeometriesRoot = colladaRoot.AddChild("library_geometries");
    XmlNode& meshNode = libraryGeometriesRoot.AddChild("geometry").AddParameter("id", meshName).AddParameter("name", meshName)
                        .AddChild("mesh");

    VertexDataMap vertexDataMap;

    while (vertexDeclaration->Stream != 0xFF)
    {
        D3DVERTEXELEMENT9* element = vertexDeclaration++;

        if (element->Stream != 0)
        {
            LOG("Vertex stream '%d' referenced", element->Stream);
            continue;
        }

        auto it_find = s_vertexDeclarationInfoMap.find(vertexDeclaration->Usage);
        if (it_find == s_vertexDeclarationInfoMap.end())
        {
            LOG("Unsupported type %d method  %d", vertexDeclaration->Type, vertexDeclaration->Method);
            continue;
        }

        size_t componentSize = GetComponentSize_(vertexDeclaration->Usage, vertexDeclaration->Type, it_find->second.defaultSize_);
        if (componentSize == 0)
        {
            LOG("Unsupported type %d method  %d", vertexDeclaration->Type, vertexDeclaration->Method);
            continue;
        }

        vertexDataMap[it_find->second.semanticName_].emplace_back(vertices + vertexDeclaration->Offset, componentSize);
    }

    for (auto& components : vertexDataMap)
    {
        for (size_t i = 0; i < components.second.size(); ++i)
        {
            VertexComponentData& vertexData = components.second[i];
            AddComponent_(meshNode, vertexData.data_, numVertices, meshName, components.first + std::to_string(i), vertexData.size_, stride);
        }
    }

    auto it_find = vertexDataMap.find("POSITION");
    if (it_find != vertexDataMap.end())
    {
        meshNode.AddChild("vertices").AddParameter("id", meshName + "-VERTEX")
            .AddChild("input").AddParameter("semantic", "POSITION").AddParameter("source", "#" + meshName + "-POSITION0");
    }

    XmlNode& primitiveNode = meshNode.AddChild(primitiveName)
                                    .AddParameter("material", meshName + "-material")
                                    .AddParameter("count", std::to_string(primitiveCount));

    if (it_find != vertexDataMap.end())
    {
        primitiveNode.AddChild("input")
            .AddParameter("semantic", "VERTEX")
            .AddParameter("source", "#" + meshName + "-VERTEX")
            .AddParameter("offset", "0");
    }

    for (auto& components : vertexDataMap)
    {
        if (components.first == "POSITION")
        {
            continue;
        }

        for (size_t i = 0; i < components.second.size(); ++i)
        {
            VertexComponentData& vertexData = components.second[i];
            primitiveNode.AddChild("input")
                .AddParameter("semantic", components.first)
                .AddParameter("source", "#" + meshName + "-" + components.first + std::to_string(i))
                .AddParameter("offset", "0");
        }
    }

    XmlNode& indiciesNode = primitiveNode.AddChild("p");
    size_t trianglesWritten = 0;
    std::string indiciesValue;
    if (indexFormat == D3DFMT_INDEX16)
    {
        const uint16_t* indexData = reinterpret_cast<const uint16_t*>(indicies);
        trianglesWritten = WriteIndicies_(indexData, numIndicies, indiciesValue, primitiveType);
        indiciesNode.SetValue(indiciesValue);
    }
    else if (indexFormat == D3DFMT_INDEX32)
    {
        const uint32_t* indexData = reinterpret_cast<const uint32_t*>(indicies);
        trianglesWritten = WriteIndicies_(indexData, numIndicies, indiciesValue, primitiveType);
    }
    else
    {
        LOG("Unsupported index format %d", indexFormat);
    }
    indiciesNode.SetValue(std::move(indiciesValue));

    XmlNode& libraryVisualScenesRoot = colladaRoot.AddChild("library_visual_scenes");
    libraryVisualScenesRoot.AddChild("visual_scene").AddParameter("id", "VisualSceneNode").AddParameter("name", meshName)
        .AddChild("node").AddParameter("id", meshName + "-node").AddParameter("name", meshName).AddParameter("type", "NODE")
        .AddChild("instance_geometry").AddParameter("url", "#" + meshName)
        .AddChild("bind_material")
        .AddChild("techinque_common")
        .AddChild("instance_material").AddParameter("symbol", meshName + "-material").AddParameter("target", "#" + meshName + "-material");
    colladaRoot.AddChild("scene")
        .AddChild("instance_visual_scene").AddParameter("url", "#VisualSceneNode");

    file << "<?xml version = \"1.0\" encoding = \"utf-8\"?>" << std::endl;
    colladaRoot.Serialize(0, file);

    m_textures.clear();

    m_primitiveNumber += 1;
}

void ColladaExporter::EndScene()
{

}
