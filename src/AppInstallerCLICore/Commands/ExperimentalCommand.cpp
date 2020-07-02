// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExperimentalCommand.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    using namespace Utility::literals;
    using namespace AppInstaller::Settings;

    using namespace std::string_view_literals;

    std::vector<Argument> ExperimentalCommand::GetArguments() const
    {
        return
        {
            Argument::ForType(Execution::Args::Type::ExperimentalArg)
        };
    }

    Resource::LocString ExperimentalCommand::ShortDescription() const
    {
        return { Resource::String::ExperimentalCommandShortDescription };
    }

    Resource::LocString ExperimentalCommand::LongDescription() const
    {
        return { Resource::String::ExperimentalCommandLongDescription };
    }

    std::string ExperimentalCommand::HelpLink() const
    {
        return "https://aka.ms/winget-settings";
    }

    void ExperimentalCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::ExperimentalArg))
        {
            static constexpr std::string_view s_ninjaCat = R"(
                                    -<vYT`                                           
                                 ')hM3d$$c                                           
                              `~uydNv>>$$$^                                          
                        -T}uymkZdqUdN3mowkh_                                         
                         T06m\xrwingetxYxcky`                                        
                          wDNf3omhXucVi!"YyjL                                        
                          `VmhkuvrHG}VVT)~:_:!           r!`-_                       
                           `hqf-.-TH3r"`     !-`r^,`-!:^)kUdE"                       
                            -jGhkyY*-        'T!YyTykzGNkxLxv-                       
,]Vyx!`                                      `"kKPGHKzhHy)*>'                        
  `_|fZV*-                           `*\*~~*LuVkXzXUm3GGx|x`      `<}cc*`            
     _ryGNm]~-                        `_"!^**vL}ycVkh5MG}|L:    !V0QMv=r|_``         
       `"rumdNbfyLr>!:",_--_""",---_!*iwHNNNZxv}XqbNNNNNNNNZX3RQBQQR98QTrrr*.        
          `"*ThZNNNNNNNNNNNNNNNNNN9NNNNNNNNNN]VNNNbNNNNdNNNQ#BBBB80N8#@QZc}}^`       
             `">vuhMNNNNNNNNNNN6$gENNNNNNR9&NKyZNNNRgd53GbN0#BBB$N60bNRNNNZ3XT!      
                 .:*]coPbNNNNNN$Q$NNNdNNN$QQ6NNNNN6QR5Xy3bNEdMMENNNbkT5NNNNN60T      
                    `-:^)YuyX3Z0QNNdMMZdN8QBQRNNN68Q6Z55dbMqqqGqdNNNUy#Ho5NNG3y      
                         `._=*vc0NZGKa5dEBBBBQQ8QQQ8NNNgGjjjjjx=:yNNNqgBN}`          
                               ~NZ3kyhq9B03oycuuymdNNN$ZvvxcqU~`  :zN}Q#8:           
                               }bHk}cmZNcxxxxxxxYVhg9N3!   .V3ZEm, .MZkZ@v           
                             `rPMKyVhG*-````-L8@@@@@@#x}3yvr:`  r&^ 'cddoX.          
                            xd#N53Um9r         m####BM   rdGY.  :PV:::,:-      __`   
                           ^NN#BZ5EQv         -QBQ$N3'   `=^.!>;,.`  `-,"::",:r!,~   
                         ~q68#@@@Dx`          rQ86Nd"           .=;~-        `x--    
                        `P9BBbx:`             ~9NNX-                _~;!' -^)*:,"    
                        rN6QZ`               `KNNZ_                    `"x*' -^      
                        yNgD5^`              XNNNy                       >)``        
                       .NNg#NQa`             ^qN$N}.                     `:r^        
                        ZNk#QNQQdx-           !N@#RgQy,-                             
                       _Zy ,h03o= `           -!G@BNQ@#85T.                          
                        `      `                :*vxL`)L! '                          
)"sv;
            context.Reporter.Info() << s_ninjaCat << std::endl;
        }
        context.Reporter.Info() << Resource::String::ThankYou << std::endl;
    }
}
