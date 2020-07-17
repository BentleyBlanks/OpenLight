#include "RenderGraphResource.h"

LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name)
{
    return LogicalResourceID();
}

LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name, const PhysicalDesc& desc)
{
    return LogicalResourceID();
}

LogicalResourceID GraphResourceMgr::CreateLogicalResource(const std::string& name, const PhysicalResourceID& physicalID)
{
    return LogicalResourceID();
}

void GraphResourceMgr::BindUnknownResource(const LogicalResourceID& logicalID, const PhysicalResourceID& physicalID)
{
}

PhysicalResourceID GraphResourceMgr::LoadPhysicalResource(const PhysicalResource& physical)
{
    return PhysicalResourceID();
}

PhysicalResourceID GraphResourceMgr::CreatePhysicalResource(const PhysicalDesc& desc)
{
    return PhysicalResourceID();
}
