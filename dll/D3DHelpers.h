#pragma once

#define MAX_TEXTURE_COUNT 8

#define D3DCALL(call)			\
    {							\
        HRESULT hr = (call);	\
        if(FAILED(hr))			\
        {						\
            LOG("d3d call failed, hr = %d", hr); \
            return;				\
        }						\
    }

template<typename ComType>
class com_ptr
{
public:
    com_ptr()
        : m_object(nullptr)
    {

    }

    com_ptr(ComType* object)
        : m_object(nullptr)
    {
        reset(object);
    }

    ~com_ptr()
    {
        if (m_object)
        {
            m_object->Release();
            m_object = nullptr;
        }
    }

    ComType* get()
    {
        return m_object;
    }

    void reset()
    {
        reset(nullptr);
    }

    void reset(ComType* object)
    {
        if (m_object)
        {
            m_object->Release();
            m_object = nullptr;
        }
        
        if (object)
        {
            object->AddRef();
            m_object = object;
        }
    }

    ComType** operator&()
    {
        return &m_object;
    }

    ComType* operator->()
    {
        return m_object;
    }

    operator bool() const
    {
        return m_object != nullptr;
    }

private:
    ComType* m_object;
};
