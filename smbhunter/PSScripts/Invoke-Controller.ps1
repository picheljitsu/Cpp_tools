function Invoke-Controller{

    [CmdletBinding()]
    Param(
        [Parameter(Position=0)]
        [Alias('p')]
        [int]$port,
        [Parameter(Mandatory=$false)]
        [string]$Output="Host",
        [parameter(Mandatory=$false)]
        [string]$ConfigFile="$PSScriptRoot\Configs"

         )

    write-verbose "Using config file: $Configfile"

    #Parse log from client. Log is sent in CSV format (no header)
    function ps_output($array_log_str){

        $array_log = $array_log_str -split (',')
        
        [string]$RawTimestamp = ($array_log[4]).trim()
        $ConvertTimeStamp = ([datetime]([System.Management.ManagementDateTimeConverter]::ToDateTime($RawTimestamp)))
        $FormattedTimeStamp = "{0:yyyy-MM-dd-hh:mm:ss}" -f $ConvertTimeStamp
        $ps_object = New-Object PSObject
        $ps_object | Add-Member -Name "Timestamp" -MemberType NoteProperty -value $FormattedTimeStamp
        $ps_object | Add-Member -Name "Source" -MemberType NoteProperty -value $array_log[0]
        $ps_object | Add-Member -Name "PID" -MemberType NoteProperty -value $array_log[1]
        $ps_object | Add-Member -Name "LocalConnection" -MemberType NoteProperty -value $array_log[2]
        $ps_object | Add-Member -Name "RemoteConnection" -MemberType NoteProperty -value $array_log[3]
        $ps_object | Add-Member -Name "ProcessName" -MemberType NoteProperty -value $array_log[5]
        $ps_object | Add-Member -Name "CommandLine" -MemberType NoteProperty -value $array_log[6]
        
        return $ps_object

        }

    clear-host
    try { write-verbose "Loading ASCII art from $configfile\ASCII_ART.txt"
          Get-Content "$ConfigFile\ASCII_ART.txt" -ErrorAction SilentlyContinue }
    catch { write-host "[-] ASCII art not found :(" -foregroundcolor Red }

    try { netsh advfirewall firewall delete rule name="ctoc $port" | Out-Null
          netsh advfirewall firewall add rule name="ctoc $port" dir=in action=allow protocol=TCP localport=$port | Out-Null
          write-host "[+] Port $port opened in firewall..."
          }
    catch { write-host "[!] Warning. Unable to establish a firewall rule.  Hosts may not be able to connect." }
    

    Write-output "[*] Starting Listener on 0.0.0.0:$port [TCP]"
    write-output "`n"
    $header = "{0,-15}{1,-20}{2,-10}{3,-25}{4,-25}{5, -20}{6}`n" -f "Source", "Timestamp", "PID","LocalConnection", "RemoteConnection", "ProcessName", "CommandLine"
    
    write-host $Header -f Red -b Black
    $array_output = @()

    $h = 0
    while($h -lt 3){

        try {

            $listener = New-object System.Net.Sockets.TcpListener $port
            $listener.Start()

            $client_connection_object = $listener.AcceptTcpClient()
            }
            catch { write-host -nonewline 
            "[!] Unable to start listener on port $port.  
            Verify that the port is not in use by another program or restart this window" 
            sleep 20
            exit

        }

        $remoteclient = $client_connection_object.Client.RemoteEndPoint.Address.IPAddressToString
        $data_stream = $client_connection_object.GetStream()
        $data_stream.ReadTimeout = [System.Threading.Timeout]::Infinite

        $receivebuffer = New-Object Byte[] $client_connection_object.ReceiveBufferSize
        $encodingtype = new-object System.Text.ASCIIEncoding


        while ($client_connection_object.Connected){

            try{ $Read = $data_stream.Read($Receivebuffer, 0, $Receivebuffer.Length) }
            catch { $output_stream = $NULL }
            if($read -eq 0){
                    $close_connection = $encodingtype.GetBytes("end")
                    $data_stream.Write($close_connection , 0, $close_connection.Length) 
                    break
                    }         
                     
            else{     
                 $close_connection = $encodingtype.GetBytes("end")
                 $data_stream.Write($close_connection , 0, $close_connection.Length) 
                 [Array]$Bytesreceived += $Receivebuffer[0..($Read -1)]
                 

                 }
            if ($data_stream.DataAvailable) {
                
                $close_connection = $encodingtype.GetBytes("end")
                $data_stream.Write($close_connection , 0, $close_connection.Length)
                
                }

            else{

                $logconnection = $EncodingType.GetString($Bytesreceived).trimend('`n`r')

                $logconnection = $logconnection -split ([char]10)

                #Replace Null bytes with spaces
                $logconnection = $logconnection -replace [char][byte]0,[char][byte]32
                 
                foreach($log in $logconnection){

                    $log = $log -replace "`r`n|`r|`n|`n`r",""

                    if($log.length -ne 0){

                        $output_stream = "$remoteclient,$log"
                        $hashentry = ps_output $output_stream
                        $array_output += $hashentry

                        #$hashentry
                        $Source           = $($hashentry.source).tostring()
                        $TimeStamp        = $($hashentry.timestamp).tostring()
                        $rPID             = $($hashentry.PID).tostring()
                        $LocalConnection  = $($hashentry.LocalConnection).tostring()
                        $RemoteConnection = $($hashentry.RemoteConnection).tostring()
                        $ProcessName      = $($hashentry.ProcessName).tostring()
                        $CommandLine      = $($hashentry.CommandLine).tostring()
                        $ConsoleOutput = $Source,$TimeStamp,$rPID,
                                         $LocalConnection,$RemoteConnection,
                                         $ProcessName,$CommandLine
                        $fOut = "{0,-15}{1,-20}{2,-10}{3,-25}{4,-25}{5, -20}{6}" -f $ConsoleOutput
                        write-host $fOut -ForegroundColor green 
                        #$output_stream
                        $h ++
                        }
                    }

                $hashentry = ''
                [Array]::Clear($Receivebuffer, 0, $Read)
                
                }



            try{
            
                if ($PSVersionTable.CLRVersion.Major -lt 4){
                    $client_connection_object.Close(); $data_stream.Close(); $listener.Stop()
                    }
                else {  write-verbose "Shutting Server Down"
                        $data_stream.Dispose(); $client_connection_object.Dispose(), $listener.Stop()}             
            
                }
            
            catch { Write-Warning "Failed to close TCP Stream"}
                                
            }

        }

    }
