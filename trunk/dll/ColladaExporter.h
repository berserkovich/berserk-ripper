#pragma once

#include "ImageCache.h"

class ColladaExporter
{
public:
    ColladaExporter(const std::string& saveFolder);
    ~ColladaExporter();

    void StartScene();
    void SetTexture(size_t stage, size_t width, size_t height, D3DFORMAT format, void* pData, int pitch);
    void AddPrimitive(D3DPRIMITIVETYPE primitiveType, 
                        const uint8_t* vertices, 
                        size_t numVertices, 
                        D3DVERTEXELEMENT9* vertexDeclaration, 
                        size_t stride, 
                        const uint8_t* indicies, 
                        size_t numIndicies, 
                        D3DFORMAT indexFormat, 
                        size_t primitiveCount);
    void EndScene();

private:
    std::string m_saveFolder;
    ImageCache m_imageCache;
};