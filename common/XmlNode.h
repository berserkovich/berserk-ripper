#pragma once

#include <list>
#include <ostream>
#include <string>
#include <vector>

class XmlNode
{
public:
    XmlNode(const std::string& name);
    XmlNode& AddChild(const std::string& name);
    XmlNode& AddParameter(const std::string& name, const std::string& value);

    template<typename T>
    XmlNode& SetValue(T&& value)
    {
        m_value = std::forward<T>(value);
        return *this;
    }

    void Serialize(std::size_t level, std::ostream& output);

private:
    std::string m_name;
    std::string m_value;
    std::list<XmlNode> m_children;
    std::vector<std::pair<std::string, std::string>> m_parameters;
};