function Build-SMBAgent{

    param(
        [parameter(Mandatory=$true)]
        [String] $path,
        [parameter(Mandatory=$false)]
        [bool] $clean = $true
        )

        $RootPath = Split-Path $path

        write-verbose $RootPath

        $MsBuildComm = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe"
        $MsBuildPro  = 'C:\Program Files (x86)\Microsoft Visual Studio\2017\Profesional\MSBuild\15.0\Bin\msbuild.exe'

        if(test-path -Path $MsBuildComm){ 

            $MsBuildExe = $MsBuildComm
            write-host "[+] Found Community MSBuild." -ForegroundColor Green
            write-host "[*] Building agent." -ForegroundColor Green
            $ret = & "$($MsBuildExe)" "$($path)" /t:Build /m
            $ret = $ret -split "`n"

            }
        else { write-host "[-] MSBuild Community version not installed." -ForegroundColor Red
               write-host "[*] Trying Professional..." -ForegroundColor Yellow 

               if(test-path -Path $MsBuildPro){

                   $msBuildExe = $MsBuildPro
                   write-host "[+] Found Professional MSBuild." -ForegroundColor Green
                   write-host "[*] Building agent." -ForegroundColor Green
                   $ret = & "$($MsBuildPro)" "$($path)" "/consoleloggerparameters:WarningsOnly /m"
                  
                   }

               else { write-host "[-] No suitable MSBuild version found." }

               }
        $ret | % { Write-Verbose "$([string]$_)" } 

        [string]$errs = $($ret | select-string -Pattern "Error.s.").ToString()

        $errs = $errs.TrimStart().Split(' ')[0]

        if($errs -eq "0"){ 
 
            $DllPath = $(Get-ChildItem -Path "$RootPath\Release" -Filter "*.dll").FullName
            write-verbose "[+] Injectable SMB DLL Agent written to: $DllPath"
            $DllObj = [Convert]::ToBase64String([IO.File]::ReadAllBytes($DllPath))
        
        
            }
        
        else { write-host "[-] Encountered errors.  Re-run with the -verbose option to troubleshoot. Stopping." -ForegroundColor Red }
            
        if($clean){  
        
            write-host "[*] Cleaning up..." -ForegroundColor Green        
            & $msBuildExe $path /t:Clean /m | Out-Null
            write-host "[+] Done." -ForegroundColor Green   
        
            }
        
        else{ write-host "[+] Skipping cleanup." -ForegroundColor Yellow }
        
        return $DllObj
     
    }
