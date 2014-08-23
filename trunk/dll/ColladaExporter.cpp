#include "ColladaExporter.h"

#include <cassert>
#include <fstream>
#include <vector>

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
size_t WriteIndicies_(const T* indexData, size_t numIndicies, std::ofstream& file, D3DPRIMITIVETYPE primitiveType)
{
    size_t trianglesWritten = 0;
    if (primitiveType == D3DPT_TRIANGLELIST)
    {
        for (size_t i = 0; i < numIndicies; ++i)
        {
            file << indexData[i] << ' ';
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
            file << idx1 << ' ' << idx2 << ' ' << idx3 << ' ';
            ++trianglesWritten;
        }
    }
    else if (primitiveType == D3DPT_TRIANGLEFAN)
    {
        // 1 2 0  3 4 0  5 6 0  7 8 0 ...
        for (size_t i = 1; i < (numIndicies - 1) / 2; i += 2)
        {
            file << indexData[i] << ' ';
            file << indexData[i + 1] << ' ';
            file << indexData[0];
            ++trianglesWritten;
        }
    }
    else
    {
        LOG("Unsupported primitive type");
    }
    return trianglesWritten;
}

void WriteComponent_(std::ofstream& file, const uint8_t* data, size_t numVertices, const std::string& meshName, const std::string& componentName, size_t componentSize, size_t stride)
{
    file << "    <source id='" << meshName << "-" << componentName << "'>\n";

    const float* componentData = reinterpret_cast<const float*>(data);
    file << "     <float_array id='" << meshName << "-" << componentName << "-array' count='" << numVertices * componentSize << "'>";
    for (size_t i = 0; i < numVertices; ++i)
    {
        for (size_t j = 0; j < componentSize; ++j)
        {
            file << componentData[j] << ' ';
        }
        file << ' ';
        componentData = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(componentData) + stride);
    }
    file << "</float_array>\n";
    file << "     <technique_common>\n";
    file << "      <accessor source = '#" << meshName << "-" << componentName << "-array' count='" << numVertices << "' stride='" << componentSize << "'>\n";
    for (size_t i = 0; i < componentSize; ++i)
    {
        file << "       <param name='" << char('X' + i) << "' type='float'/>\n";
    }
    file << "      </accessor>\n";
    file << "     </technique_common>\n";
    file << "    </source>\n";
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
{
    static bool vertexDeclarationInit = false;
    if (!vertexDeclarationInit)
    {
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_POSITION, VertexDeclarationInfo(std::string("POSITION"), 3)));
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_NORMAL, VertexDeclarationInfo(std::string("NORMAL"), 3)));
        s_vertexDeclarationInfoMap.insert(std::make_pair(D3DDECLUSAGE_TEXCOORD, VertexDeclarationInfo(std::string("TEXCOORD"), 2)));
        vertexDeclarationInit = true;
    }
}

ColladaExporter::~ColladaExporter()
{

}

void ColladaExporter::StartScene()
{

}

void ColladaExporter::SetTexture(size_t stage, size_t width, size_t height, D3DFORMAT format, void* pData, int pitch)
{
    m_imageCache.Add(width, height, format, pData, pitch);
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
    static int count = 0;
    std::ofstream file;

    std::string filename = m_saveFolder + "\\primitive_" + std::to_string(count) + ".dae";
    file.open(filename, std::ios_base::out | std::ios_base::binary);
    file.precision(10);

    std::string meshName = "mesh_" + std::to_string(count);
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

    file << "<?xml version='1.0' encoding='utf-8'?>\n";
    file << "<COLLADA xmlns='http://www.collada.org/2005/11/COLLADASchema' version='1.4.1'>\n";
    file << " <asset>\n";
    file << "  <unit meter='0.01' name='centimeter'/>\n";
    file << " </asset>\n";
    file << " <library_geometries>\n";
    file << "  <geometry id='" << meshName << "' name='" << meshName << "'>\n";
    file << "   <mesh>\n";

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
            WriteComponent_(file, vertexData.data_, numVertices, meshName, components.first + std::to_string(i), vertexData.size_, stride);
        }
    }


    auto it_find = vertexDataMap.find("POSITION");
    if (it_find != vertexDataMap.end())
    {
        file << "    <vertices id='" << meshName << "-VERTEX'>\n";
        file << "     <input semantic='POSITION' source='#" << meshName << "-POSITION0'/>\n";
        file << "    </vertices>\n";
    }


    file << "    <" << primitiveName << " count='" << primitiveCount << "'>\n";

    if (it_find != vertexDataMap.end())
    {
        file << "     <input semantic='VERTEX' source='#" << meshName << "-VERTEX' offset = '0'/>\n";
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
            file << "     <input semantic='" << components.first << "' source='#" << meshName << "-" << components.first + std::to_string(i) << "' offset = '0'/>\n";
        }
    }

    file << "     <p>";
    size_t trianglesWritten = 0;
    if (indexFormat == D3DFMT_INDEX16)
    {
        const uint16_t* indexData = reinterpret_cast<const uint16_t*>(indicies);
        trianglesWritten = WriteIndicies_(indexData, numIndicies, file, primitiveType);
    }
    else if (indexFormat == D3DFMT_INDEX32)
    {
        const uint32_t* indexData = reinterpret_cast<const uint32_t*>(indicies);
        trianglesWritten = WriteIndicies_(indexData, numIndicies, file, primitiveType);
    }
    else
    {
        LOG("Unsupported index format %d", indexFormat);
    }

    file << "</p>\n";

    //file << "     <vcount>";
    //for (size_t i = 0; i < trianglesWritten; ++i)
    //{
    //    file << "3 ";
    //}
    //file << "</vcount>\n";

    file << "    </" << primitiveName << ">\n";


    file << "   </mesh>\n";
    file << "  </geometry>\n";
    file << " </library_geometries>\n";
    file << " <library_visual_scenes>\n";
    file << "  <visual_scene id='VisualSceneNode' name='" << meshName << "'>\n";
    file << "   <node id='" << meshName << "-node' name='" << meshName << "' type='NODE'>\n";
    file << "    <instance_geometry url='#" << meshName << "'/>\n";
    file << "   </node>\n";
    file << "  </visual_scene>\n";
    file << " </library_visual_scenes>\n";
    file << " <scene>\n";
    file << "  <instance_visual_scene url='#VisualSceneNode'/>\n";
    file << " </scene>\n";
    file << "</COLLADA>\n";

    count += 1;
}

void ColladaExporter::EndScene()
{

}
