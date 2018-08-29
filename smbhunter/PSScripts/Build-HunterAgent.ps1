function Build-HunterAgent{

    param(
        [ValidateScript({ if(!$(Test-Path $_) ){ throw "Invalid Solution File" }; return $true })]
        [parameter(Mandatory=$true)]
        [System.IO.FileInfo] $Path,
        [parameter(Mandatory=$false)]
        [bool] $clean = $true,
        [parameter(Mandatory=$false)]
        [System.IO.FileInfo]$MSBuildPath
        )

        write-host "[*] Starting Build process..." -ForegroundColor Green

        $RootPath = Split-Path (get-childitem $path)

        write-verbose "Script root path: $($RootPath)"

        #Add MSBuild Paths here
        if(!$MSBuildPath) {

            $MsBuildComm = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe"
            $MsBuildPro  = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\msbuild.exe"
        
            }

        $MSBuildExe = ''

        #Tested only on community edition. Will be used in preference over Professional.
        if(test-path -Path $MsBuildComm){ 
                    
            $MsBuildExe = $MsBuildComm
            $Msg = "[+] Found Community MSBuild."
           
            }

        else { write-host "[-] MSBuild Community version not found." -ForegroundColor Red
               write-host "[*] Trying Professional..." -ForegroundColor Yellow 

               if(test-path -Path $MsBuildPro){

                   $MsBuildExe = $MsBuildPro
                   $Msg = "[+] Found Professional MSBuild."
                                     
                   }

               else { write-host "[-] No suitable MSBuild version found." }

               }
        #If MSBuild is found, compile the solution file (.sln), logging warnings and enabling multi-threading
        if($MsBuildExe){ 

            write-host $Msg -ForegroundColor Green 
            write-host "[*] Compiling agent." -ForegroundColor Green            
            $ret = & "$($MsBuildExe)" "$($Path)" /consoleloggerparameters:WarningsOnly /m
            $ret = $ret -split "`n"

            }

        #Didn't get anything returned so assume an error occurred.
        if(!$ret){ 
            
            write-host "[-] Something went wrong. Couldn't grab output from compile attempt with MSBuild." -ForegroundColor red 
            
            }

        else { #Parse out errors for the MSBuild run. Warnings are expected.
               $ret | % { Write-Verbose "$([string]$_)" } 
               [string]$errs = $($ret | select-string -Pattern "Error.s.").ToString()
               $errs = $errs.TrimStart().Split(' ')[0]
               
               #No errors received, convert the dll into a base 64 string
               if($errs -eq "0"){ 

                   write-host "[+] Agent compilation completed successfully."
                   $DllPath = $(Get-ChildItem -Path "$RootPath\x64\Release" -Filter "*.dll").FullName
                   $Md5Hash = Get-FileHash -Algorithm MD5 $DLLPath
                   write-verbose "[+] Injectable .dll Agent written to: $DllPath"
                   write-verbose "[*] Generating Base 64 encoded string of .dll" 
                   $DllObj = [Convert]::ToBase64String([IO.File]::ReadAllBytes($DllPath))
               
                   }
               
               #Errors received.  
               else { write-host "[-] Encountered errors.  Re-run with the -verbose option to troubleshoot. Stopping." -ForegroundColor Red }
                   
               #Clean up the output if set and we received a ret value
               if($clean -and $ret -and $($errs -eq "0")){  
               
                   write-host "[*] Cleaning up..." -ForegroundColor Green        
                   & $msBuildExe $path /t:Clean /m | Out-Null
  
               
                   }
               
               else{ write-host "[+] Skipping cleanup." -ForegroundColor Yellow }
               
               Write-Verbose $DllObj
               write-host "[+] MD5 Hash: $($Md5Hash.hash)" -ForegroundColor Green 
               write-host "[+] Done." -ForegroundColor Green  
                             
               }
     
    }


