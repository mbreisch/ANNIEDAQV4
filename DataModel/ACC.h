#ifndef ACC_H
#define ACC_H

class ACC 
{
    public:
        virtual void someFunction() = 0; // Pure virtual function, must be implemented by derived classes
        virtual ~ACC() {} // Virtual destructor to ensure proper cleanup in case of polymorphic deletion
};

#endif 