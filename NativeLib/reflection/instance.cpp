#include "instance.hpp"

// seems like crap to me. x2
auto reflection::RbxInstance::FindFirstChildOfClass(const std::string& class_name, bool recursive) -> RbxInstance
{
    Instance_t* Parent = this->inst_ptr;

    if ( !Parent->Children ) {
        return RbxInstance{ nullptr };
    }
    
    const auto children = Parent->Children.get( );
    for (auto child : *children) {
        std::string name = child->class_descriptor->class_name;

        if (name == class_name) {
            return RbxInstance{ child.get( ) };
        }
        else if (recursive) {
            auto ChildInst = RbxInstance{ child.get( ) };
            
            auto second_child = ChildInst.FindFirstChildOfClass(class_name, recursive);
            return second_child;
        }
    }

    return RbxInstance{ nullptr };
}

auto reflection::RbxInstance::FindFirstChild(const std::string& child_name, bool recursive) -> RbxInstance
{
    Instance_t* Parent = this->inst_ptr;

    if ( !Parent->Children ) {
        return RbxInstance{ nullptr };
    }
    
    const auto children = Parent->Children.get( );
    for (auto child : *children) {
        std::string name = child->name;

        if (name == child_name)
            return RbxInstance{ child.get( ) };
        else if (recursive) {
            auto ChildInst = RbxInstance{ child.get( ) };
            
            auto second_child = ChildInst.FindFirstChild(child_name, recursive);
            return second_child;
        }
    }
    
    return RbxInstance{ nullptr };
}

auto reflection::RbxInstance::GetChildren( ) -> std::vector<RbxInstance>
{
    std::vector<RbxInstance> ChildrenList;
    Instance_t* Parent = this->inst_ptr;

    if ( !Parent->Children ) {
        return ChildrenList;
    }
    
    const auto children = Parent->Children.get( );
    for (auto child : *children) {
        ChildrenList.push_back(RbxInstance{ child.get( ) });
    }
    
    return ChildrenList;
}

auto reflection::RbxInstance::GetFullName( ) -> std::string
{
    auto inst = this->inst_ptr;
    
    if ( inst->Parent )
        return RbxInstance{ inst->Parent }.GetFullName( ) + "." + this->GetName( );
    else
        return this->GetName( );
}

auto reflection::RbxInstance::GetPropertyDescriptor(std::uintptr_t ktablecode) -> std::uintptr_t*
{
    // pseudocode, shoulve kept them unnamed
    auto inst = this->inst_ptr;
    auto classdesc = reinterpret_cast<std::uintptr_t>(inst->class_descriptor);
    
    auto descriptors_begin = *reinterpret_cast<std::uintptr_t*>(classdesc + 460);
    auto descriptors_end = *reinterpret_cast<std::uintptr_t*>(classdesc + 464);
    bool is_end = descriptors_end == descriptors_begin;

    if ( descriptors_end != descriptors_begin )
    {
        classdesc = *reinterpret_cast<std::uintptr_t*>(classdesc + 476);
        is_end = classdesc == ktablecode;
    }
    
    if ( is_end )
        return 0;
    
    unsigned int v8 = ktablecode + (ktablecode >> 3);
    unsigned int index = 1;
    unsigned int v10 = -1431655765 * ((unsigned int)(descriptors_end - descriptors_begin) >> 2) - 1;
   
    int v11;
    std::uintptr_t* v12;  
    while ( 1 )
    {
        v11 = v8 & v10;
        v12 = reinterpret_cast<std::uintptr_t*>(12 * v11 + descriptors_begin);
        if ( *v12 == ktablecode )
            break;

        if ( *v12 != classdesc )
        {
            v8 = index + v11;
            if ( index++ <= v10 )
                continue;
        }
        break;
    }

    std::uintptr_t* prop = v12 + 1;
    bool bad_property = v12 + 1 == nullptr;
    if ( v12 != reinterpret_cast<std::uintptr_t*>(-4) )
    {
        prop = reinterpret_cast<std::uintptr_t*>(*prop);
        bad_property = prop == nullptr;
    }
  
    if ( bad_property )
          return nullptr;

    return prop;
}