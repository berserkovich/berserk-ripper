#include "common/XmlNode.h"

XmlNode::XmlNode(const std::string& name)
    : m_name(name)
{

}

XmlNode& XmlNode::AddChild(const std::string& name)
{
    m_children.push_back(XmlNode(name));
    return m_children.back();
}

XmlNode& XmlNode::AddParameter(const std::string& name, const std::string& value)
{
    m_parameters.push_back(std::make_pair(name, value));
    return *this;
}

void XmlNode::Serialize(std::size_t level, std::ostream& output)
{
    std::string indent(level, ' ');
    output << indent << "<" << m_name;
    for (auto& param : m_parameters)
    {
        output << " " << param.first << "=\"" << param.second << "\"";
    }

    if (m_children.empty() && m_value.empty())
    {
        output << "/>" << std::endl;
        return;
    }

    if (!m_value.empty())
    {
        output << ">" << m_value << "</" << m_name << ">" << std::endl;
        return;
    }

    output << ">" << std::endl;
    for (auto& child : m_children)
    {
        child.Serialize(level + 1, output);
    }

    output << indent << "</" << m_name << ">" << std::endl;
}
