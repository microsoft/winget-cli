// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Opens the pinning index for use in future operations.
    // Required Args: None
    // Inputs: None
    // Outputs: PinningIndex
    void OpenPinningIndex(Execution::Context& context);

    // Gets all the pins from the index.
    // Required Args: None
    // Inputs: PinningIndex
    // Outputs: Pins
    void GetAllPins(Execution::Context& context);

    // Searches for all the pins associated with a package.
    // There may be several if a package is available from multiple sources.
    // Required Args: None
    // Inputs: PinningIndex, Package
    // Outputs Pins
    void SearchPin(Execution::Context& context);

    // Adds a pin for the current package.
    // A separate pin will be added for each source.
    // Required Args: None
    // Inputs: PinningIndex, Package
    // Outputs: None
    void AddPin(Execution::Context& context);

    // Removes all the pins associated with a package.
    void RemovePin(Execution::Context& context);

    // Report the pins in a table.
    // Required Args: None
    // Inputs: Pins
    // Outputs: None
    void ReportPins(Execution::Context& context);

    // Resets all the existing pins.
    // Required Args: None
    // Inputs: PinningIndex
    // Outputs: None
    void ResetAllPins(Execution::Context& context);

    // Updates the list of pins to include only those matching the current open source
    // Required Args: None
    // Inputs: Pins, Source
    // Outputs: None
    void CrossReferencePinsWithSource(Execution::Context& context);
}
