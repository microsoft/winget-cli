// Shows the contents of a configuration yml file
using Windows.Storage;
using Microsoft.Management.Configuration;
using Microsoft.Management.Configuration.Processor;

if (args.Length != 1)
{
    Console.WriteLine("Usage: Configuration-InProc <path>");
    return;
}

var configStatics = new ConfigurationStaticFunctions();
if (!configStatics.IsConfigurationAvailable)
{
    throw new Exception("Configuration is not available");
}

// Differs from the OutOfProc version in that we create the factory object directly
var factory = new PowerShellConfigurationSetProcessorFactory();

var processor = configStatics.CreateConfigurationProcessor(factory);

var file = await StorageFile.GetFileFromPathAsync(args[0]);
var fileStream = await file.OpenStreamForReadAsync();

var openResult = processor.OpenConfigurationSet(fileStream.AsInputStream());
var configSet = openResult.Set;

Console.WriteLine("Configuration set:");
configSet.Serialize(Console.OpenStandardOutput().AsOutputStream());
