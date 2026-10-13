#ifndef NETWORKDIAGNOSTICS_H
#define NETWORKDIAGNOSTICS_H

#include <QString>

// Runs the bundled per-platform network diagnostics script (extracted from a
// Qt resource into a securely-created temp dir) and returns its concatenated
// section output, or an "ERROR: ..." string on failure.
class NetworkDiagnostics
{
public:
    static QString run();
};

#endif // NETWORKDIAGNOSTICS_H
