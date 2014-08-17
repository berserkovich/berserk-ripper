#include "ColladaExporter.h"

#include <cassert>
#include <fstream>

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
            if ((idx1 == idx2) && (idx1 == idx3))
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


ColladaExporter::ColladaExporter(const std::string& saveFolder)
    : m_saveFolder(saveFolder)
    , m_imageCache(saveFolder)
{

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
        primitiveName = "polylist";
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

    while (vertexDeclaration->Stream != 0xFF)
    {
        D3DVERTEXELEMENT9* element = vertexDeclaration++;

        if (element->Stream != 0)
        {
            LOG("Vertex stream '%d' referenced", element->Stream);
            continue;
        }

        if (vertexDeclaration->Usage == D3DDECLUSAGE_POSITION)
        {
            file << "    <source id=\"" << meshName << "-positions\" name=\"position\">\n";
            if (vertexDeclaration->Type == D3DDECLTYPE_FLOAT3
                || vertexDeclaration->Type == D3DDECLTYPE_UNUSED)
            {
                const float* positionData = reinterpret_cast<const float*>(vertices + vertexDeclaration->Offset);
                file << "     <float_array id=\"" << meshName << "-position-data\" count=\"" << numVertices * 3 << "\">";
                for (size_t i = 0; i < numVertices; ++i)
                {
                    file << ' ' << positionData[0] << ' ' << positionData[1] << ' ' << positionData[2] << ' ';
                    positionData = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(positionData) + stride + vertexDeclaration->Offset);
                }
                file << "</float_array>\n";
                file << "     <technique_common>\n";
                file << "      <accessor source = \"#" << meshName << "-position-data\" count=\"" << numVertices << "\" stride=\"" << 3 << "\">\n";
                file << "       <param name=\"X\" type=\"float\"/>\n";
                file << "       <param name=\"Y\" type=\"float\"/>\n";
                file << "       <param name=\"Z\" type=\"float\"/>\n";
                file << "      </accessor>\n";
                file << "     </technique_common>\n";
                file << "    </source>\n";
            }
            else
            {
                LOG("Unsupported type %d", vertexDeclaration->Type);
            }
        }
        else if (vertexDeclaration->Usage == D3DDECLUSAGE_NORMAL)
        {
            file << "    <source id=\"" << meshName << "-normals\" name=\"normal\">\n";
            if (vertexDeclaration->Type == D3DDECLTYPE_FLOAT3
                || vertexDeclaration->Type == D3DDECLTYPE_UNUSED)
            {
                const float* normalData = reinterpret_cast<const float*>(vertices + vertexDeclaration->Offset);
                file << "     <float_array id=\"" << meshName << "-normal-data\" count=\"" << numVertices * 3 << "\">";
                for (size_t i = 0; i < numVertices; ++i)
                {
                    file << ' ' << normalData[0] << ' ' << normalData[1] << ' ' << normalData[2] << ' ';
                    normalData = reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(normalData)+stride + vertexDeclaration->Offset);
                }
                file << "</float_array>\n";
                file << "     <technique_common>\n";
                file << "      <accessor source = \"#" << meshName << "-normal-data\" count=\"" << numVertices << "\" stride=\"" << 3 << "\">\n";
                file << "       <param name=\"X\" type=\"float\"/>\n";
                file << "       <param name=\"Y\" type=\"float\"/>\n";
                file << "       <param name=\"Z\" type=\"float\"/>\n";
                file << "      </accessor>\n";
                file << "     </technique_common>\n";
                file << "    </source>\n";
            }
            else
            {
                LOG("Unsupported type %d", vertexDeclaration->Type);
            }
        }
        else
        {
            LOG("Unsupported type %d method  %d", vertexDeclaration->Type, vertexDeclaration->Method);
        }
    }

    file << "    <vertices id=\"" << meshName << "-vertices\">\n";
    file << "     <input semantic=\"POSITION\" source=\"#" << meshName << "-positions\"/>\n";
    file << "    </vertices>\n";

    file << "    <" << primitiveName << " material=\"default\" count=\"" << primitiveCount << "\">\n";
    file << "     <input semantic=\"VERTEX\" source=\"#" << meshName << "-vertices\" offset = \"0\"/>\n";
    file << "     <input semantic=\"NORMAL\" source=\"#" << meshName << "-normals\" offset = \"0\"/>\n";
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

    file << "     <vcount>";
    for (size_t i = 0; i < trianglesWritten; ++i)
    {
        file << "3 ";
    }
    file << "</vcount>\n";

    file << "    </" << primitiveName << ">\n";


    file << "   </mesh>\n";
    file << "  </geometry>\n";
    file << " </library_geometries>\n";
    file << " <library_visual_scenes>\n";
    file << "  <visual_scene id=\"VisualSceneNode\" name=\"" << meshName << "\">\n";
    file << "   <node id=\"" << meshName << "-node\" name=\"" << meshName << "\" type=\"NODE\">\n";
    file << "    <instance_geometry url=\"#" << meshName << "\"/>\n";
    file << "   </node>\n";
    file << "  </visual_scene>\n";
    file << " </library_visual_scenes>\n";
    file << " <scene>\n";
    file << "  <instance_visual_scene url=\"#VisualSceneNode\"/>\n";
    file << " </scene>\n";
    file << "</COLLADA>\n";

    count += 1;
}

void ColladaExporter::EndScene()
{

}
