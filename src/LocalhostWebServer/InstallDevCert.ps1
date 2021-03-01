param(
    [Parameter(Mandatory=$true)]
    [string]$pfxpath,

    [Parameter(Mandatory=$true)]
    [string]$password
)

Add-Type -AssemblyName System.Security
$cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
$cert.Import($pfxpath, $password, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]"PersistKeySet")
$store = new-object system.security.cryptography.X509Certificates.X509Store -argumentlist "Root", LocalMachine
$store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]"ReadWrite")
$store.Add($cert)
$store.Close()