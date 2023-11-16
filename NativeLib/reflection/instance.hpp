#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace reflection 
{
    struct property_descriptor_t // reference 
    {
        void* vftable; // 0
        std::string& name; // 4
        
        std::uint8_t pad_0[24]; // 8
        
        std::uintptr_t isscriptable; // 32
    };
    
    struct class_descriptor_t
    {
        void* vftable; // 0
        std::string& class_name; // 4
        
        std::uint8_t pad_0[28]; // 8
        
        // bruh?
        std::uintptr_t properties_start; // 36
        std::uintptr_t properties_end; // 40
        
        std::uint32_t id; // 44
    };
    
    struct Instance_t
    {
        void* vftable; // 0
        std::shared_ptr<Instance_t> self; // 4, 8
        class_descriptor_t* class_descriptor; // 12
    
        std::uint8_t pad_0[15]; // 12 - 30
        
        bool archivable; // 31
        bool isparentlocked; // 32
        bool robloxLocked; // 33
        bool issettingparent; // 34
        
        std::uint8_t pad_1[2]; // 34 - 36
    
        std::string& name; // 40
        std::shared_ptr<std::vector<std::shared_ptr<Instance_t>>> Children; // 44, 48
    
        Instance_t* Parent; // 52
    };
    
    // seems like crap to me.
    class RbxInstance {
    private:
        Instance_t* inst_ptr = nullptr;
        
    public:
        RbxInstance(std::uintptr_t address) : inst_ptr{ reinterpret_cast<Instance_t*>(address) } { }
        RbxInstance(Instance_t* address) : inst_ptr{ address } { }
        
        auto IsEmpty( ) -> bool {
            return (this->inst_ptr == nullptr);
        }
        
        auto IsArchivable( ) -> bool {
            return this->inst_ptr->archivable;
        }
        
        auto IsParentLocked( ) -> bool {
            return this->inst_ptr->isparentlocked;
        }
        
        auto IsRobloxLocked( ) -> bool {
            return this->inst_ptr->robloxLocked;
        }
        
        auto IsSettingParent( ) -> bool {
            return this->inst_ptr->issettingparent;
        }
        
        auto GetName( ) -> std::string {
            return this->inst_ptr->name;
        }
        
        auto GetClassName( ) -> std::string {
            return this->inst_ptr->class_descriptor->class_name;
        }
        
        auto GetParent( ) -> RbxInstance {
            return this->inst_ptr->Parent;
        }
        
        auto FindFirstChildOfClass(const std::string& class_name, bool recursive) -> RbxInstance;
        auto FindFirstChild(const std::string& child_name, bool recursive) -> RbxInstance;
        auto GetChildren( ) -> std::vector<RbxInstance>;
        auto GetFullName( ) -> std::string;
        auto GetPropertyDescriptor(std::uintptr_t ktablecode) -> std::uintptr_t*;
    };
}