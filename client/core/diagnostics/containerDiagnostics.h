#ifndef CONTAINERDIAGNOSTICS_H
#define CONTAINERDIAGNOSTICS_H

namespace amnezia
{
    struct ContainerDiagnostics
    {
        bool available = false;
        bool portReachable = false;

        virtual ~ContainerDiagnostics() = default;
    };

} // namespace amnezia

#endif // CONTAINERDIAGNOSTICS_H
