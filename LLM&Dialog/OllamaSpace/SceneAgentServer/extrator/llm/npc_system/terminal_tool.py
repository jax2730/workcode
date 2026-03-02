"""
TerminalTool - 安全的文件系统访问工具
基于 HelloAgents 第9章设计

功能:
- 安全的命令行执行
- 文件系统探索
- 文本文件读取
- 多层安全机制
"""

import os
import subprocess
import shlex
from pathlib import Path
from datetime import datetime
from dataclasses import dataclass
from typing import List, Dict, Any, Optional, Set


# ==================== 配置 ====================

@dataclass
class TerminalConfig:
    """终端工具配置"""
    workspace: str = "."              # 工作目录(沙箱)
    timeout: int = 30                 # 命令超时(秒)
    max_output_size: int = 10 * 1024  # 最大输出大小(10KB)
    allow_cd: bool = True             # 是否允许cd命令
    
    # 命令白名单
    allowed_commands: Set[str] = None
    
    def __post_init__(self):
        if self.allowed_commands is None:
            self.allowed_commands = {
                # 文件列表与信息
                'ls', 'dir', 'tree',
                # 文件内容查看
                'cat', 'head', 'tail', 'less', 'more', 'type',
                # 文件搜索
                'find', 'grep', 'egrep', 'fgrep', 'findstr',
                # 文本处理
                'wc', 'sort', 'uniq', 'cut', 'awk', 'sed',
                # 目录操作
                'pwd', 'cd',
                # 文件信息
                'file', 'stat', 'du', 'df',
                # 其他
                'echo', 'which', 'whereis', 'where',
            }


# ==================== TerminalTool ====================

class TerminalTool:
    """
    终端工具 - 安全的文件系统访问
    
    安全机制:
    1. 命令白名单 - 只允许安全的只读命令
    2. 工作目录限制 - 只能访问指定目录
    3. 超时控制 - 防止无限循环
    4. 输出大小限制 - 防止内存溢出
    """
    
    def __init__(self, config: TerminalConfig = None, workspace: str = None):
        if config:
            self.config = config
        else:
            self.config = TerminalConfig(workspace=workspace or ".")
        
        # 解析工作目录
        self.workspace = Path(self.config.workspace).resolve()
        if not self.workspace.exists():
            self.workspace.mkdir(parents=True, exist_ok=True)
        
        self.current_dir = self.workspace
        
        print(f"[TerminalTool] 工作目录: {self.workspace}")
    
    def execute(self, action: str = "run", **kwargs) -> str:
        """执行操作"""
        if action == "run":
            return self._run_command(kwargs.get("command", ""))
        elif action == "read":
            return self._read_file(kwargs.get("path", ""))
        elif action == "list":
            return self._list_dir(kwargs.get("path", "."))
        elif action == "search":
            return self._search_files(
                kwargs.get("pattern", "*"),
                kwargs.get("path", ".")
            )
        elif action == "pwd":
            return str(self.current_dir)
        else:
            raise ValueError(f"未知操作: {action}")
    
    def run(self, params: Dict[str, Any]) -> str:
        """运行命令 (兼容接口)"""
        command = params.get("command", "")
        return self._run_command(command)
    
    def _run_command(self, command: str) -> str:
        """执行命令"""
        if not command:
            return "❌ 命令为空"
        
        # 解析命令
        try:
            parts = shlex.split(command)
        except ValueError as e:
            return f"❌ 命令解析失败: {e}"
        
        if not parts:
            return "❌ 命令为空"
        
        cmd_name = parts[0].lower()
        
        # 处理Windows命令别名
        cmd_aliases = {
            'dir': 'ls',
            'type': 'cat',
            'findstr': 'grep',
            'where': 'which',
        }
        cmd_name = cmd_aliases.get(cmd_name, cmd_name)
        
        # 检查命令白名单
        if cmd_name not in self.config.allowed_commands:
            allowed_list = ', '.join(sorted(self.config.allowed_commands))
            return f"❌ 不允许的命令: {parts[0]}\n允许的命令: {allowed_list}"
        
        # 特殊处理cd命令
        if cmd_name == 'cd':
            return self._handle_cd(parts)
        
        # 检查路径安全性
        safe_command = self._sanitize_command(parts)
        if safe_command.startswith("❌"):
            return safe_command
        
        # 执行命令
        return self._execute_shell(safe_command)
    
    def _handle_cd(self, parts: List[str]) -> str:
        """处理cd命令"""
        if not self.config.allow_cd:
            return "❌ cd命令已禁用"
        
        if len(parts) < 2:
            return f"当前目录: {self.current_dir}"
        
        target = parts[1]
        
        # 处理特殊路径
        if target == "..":
            new_dir = self.current_dir.parent
        elif target == ".":
            new_dir = self.current_dir
        elif target == "~":
            new_dir = self.workspace
        elif target.startswith("/") or (len(target) > 1 and target[1] == ':'):
            # 绝对路径
            new_dir = Path(target).resolve()
        else:
            # 相对路径
            new_dir = (self.current_dir / target).resolve()
        
        # 检查是否在工作目录内
        try:
            new_dir.relative_to(self.workspace)
        except ValueError:
            return f"❌ 不允许访问工作目录外的路径: {new_dir}"
        
        # 检查目录是否存在
        if not new_dir.exists():
            return f"❌ 目录不存在: {new_dir}"
        
        if not new_dir.is_dir():
            return f"❌ 不是目录: {new_dir}"
        
        self.current_dir = new_dir
        return f"✅ 切换到目录: {self.current_dir}"
    
    def _sanitize_command(self, parts: List[str]) -> str:
        """检查和清理命令中的路径"""
        sanitized_parts = [parts[0]]
        
        for part in parts[1:]:
            # 跳过选项参数
            if part.startswith('-'):
                sanitized_parts.append(part)
                continue
            
            # 检查是否是路径
            if '/' in part or '\\' in part or part == '.' or part == '..':
                # 解析路径
                if part.startswith('/') or (len(part) > 1 and part[1] == ':'):
                    # 绝对路径
                    full_path = Path(part).resolve()
                else:
                    # 相对路径
                    full_path = (self.current_dir / part).resolve()
                
                # 检查是否在工作目录内
                try:
                    full_path.relative_to(self.workspace)
                    sanitized_parts.append(str(full_path))
                except ValueError:
                    return f"❌ 不允许访问工作目录外的路径: {part}"
            else:
                sanitized_parts.append(part)
        
        return ' '.join(sanitized_parts)
    
    def _execute_shell(self, command: str) -> str:
        """执行shell命令"""
        try:
            # 根据操作系统选择shell
            if os.name == 'nt':  # Windows
                result = subprocess.run(
                    command,
                    shell=True,
                    cwd=str(self.current_dir),
                    capture_output=True,
                    text=True,
                    timeout=self.config.timeout,
                    encoding='utf-8',
                    errors='replace'
                )
            else:  # Unix/Linux/Mac
                result = subprocess.run(
                    command,
                    shell=True,
                    cwd=str(self.current_dir),
                    capture_output=True,
                    text=True,
                    timeout=self.config.timeout
                )
            
            # 合并输出
            output = result.stdout
            if result.stderr:
                output += f"\n[stderr]\n{result.stderr}"
            
            # 检查输出大小
            if len(output) > self.config.max_output_size:
                output = output[:self.config.max_output_size]
                output += f"\n\n⚠️ 输出被截断（超过 {self.config.max_output_size} 字节）"
            
            # 添加返回码信息
            if result.returncode != 0:
                output = f"⚠️ 命令返回码: {result.returncode}\n\n{output}"
            
            return output if output.strip() else "✅ 命令执行成功（无输出）"
            
        except subprocess.TimeoutExpired:
            return f"❌ 命令执行超时（超过 {self.config.timeout} 秒）"
        except Exception as e:
            return f"❌ 命令执行失败: {e}"
    
    def _read_file(self, path: str) -> str:
        """读取文件内容"""
        if not path:
            return "❌ 路径为空"
        
        # 解析路径
        if path.startswith('/') or (len(path) > 1 and path[1] == ':'):
            full_path = Path(path).resolve()
        else:
            full_path = (self.current_dir / path).resolve()
        
        # 检查是否在工作目录内
        try:
            full_path.relative_to(self.workspace)
        except ValueError:
            return f"❌ 不允许访问工作目录外的路径: {path}"
        
        # 检查文件是否存在
        if not full_path.exists():
            return f"❌ 文件不存在: {path}"
        
        if not full_path.is_file():
            return f"❌ 不是文件: {path}"
        
        # 读取文件
        try:
            with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read(self.config.max_output_size)
            
            if len(content) >= self.config.max_output_size:
                content += f"\n\n⚠️ 文件内容被截断（超过 {self.config.max_output_size} 字节）"
            
            return content
        except Exception as e:
            return f"❌ 读取文件失败: {e}"
    
    def _list_dir(self, path: str = ".") -> str:
        """列出目录内容"""
        # 解析路径
        if path.startswith('/') or (len(path) > 1 and path[1] == ':'):
            full_path = Path(path).resolve()
        else:
            full_path = (self.current_dir / path).resolve()
        
        # 检查是否在工作目录内
        try:
            full_path.relative_to(self.workspace)
        except ValueError:
            return f"❌ 不允许访问工作目录外的路径: {path}"
        
        # 检查目录是否存在
        if not full_path.exists():
            return f"❌ 目录不存在: {path}"
        
        if not full_path.is_dir():
            return f"❌ 不是目录: {path}"
        
        # 列出内容
        try:
            items = []
            for item in sorted(full_path.iterdir()):
                if item.is_dir():
                    items.append(f"📁 {item.name}/")
                else:
                    size = item.stat().st_size
                    if size < 1024:
                        size_str = f"{size}B"
                    elif size < 1024 * 1024:
                        size_str = f"{size/1024:.1f}KB"
                    else:
                        size_str = f"{size/1024/1024:.1f}MB"
                    items.append(f"📄 {item.name} ({size_str})")
            
            if not items:
                return "目录为空"
            
            return f"目录: {full_path}\n\n" + "\n".join(items)
        except Exception as e:
            return f"❌ 列出目录失败: {e}"
    
    def _search_files(self, pattern: str, path: str = ".") -> str:
        """搜索文件"""
        # 解析路径
        if path.startswith('/') or (len(path) > 1 and path[1] == ':'):
            full_path = Path(path).resolve()
        else:
            full_path = (self.current_dir / path).resolve()
        
        # 检查是否在工作目录内
        try:
            full_path.relative_to(self.workspace)
        except ValueError:
            return f"❌ 不允许访问工作目录外的路径: {path}"
        
        # 搜索文件
        try:
            matches = list(full_path.rglob(pattern))
            
            if not matches:
                return f"未找到匹配 '{pattern}' 的文件"
            
            # 限制结果数量
            max_results = 50
            results = []
            for match in matches[:max_results]:
                try:
                    rel_path = match.relative_to(full_path)
                    if match.is_dir():
                        results.append(f"📁 {rel_path}/")
                    else:
                        results.append(f"📄 {rel_path}")
                except:
                    pass
            
            output = f"搜索 '{pattern}' 在 {full_path}\n找到 {len(matches)} 个匹配\n\n"
            output += "\n".join(results)
            
            if len(matches) > max_results:
                output += f"\n\n⚠️ 只显示前 {max_results} 个结果"
            
            return output
        except Exception as e:
            return f"❌ 搜索失败: {e}"
    
    # ==================== 便捷方法 ====================
    
    def ls(self, path: str = ".") -> str:
        """列出目录"""
        return self._list_dir(path)
    
    def cat(self, path: str) -> str:
        """读取文件"""
        return self._read_file(path)
    
    def pwd(self) -> str:
        """当前目录"""
        return str(self.current_dir)
    
    def cd(self, path: str) -> str:
        """切换目录"""
        return self._handle_cd(['cd', path])
    
    def find(self, pattern: str, path: str = ".") -> str:
        """搜索文件"""
        return self._search_files(pattern, path)
    
    def head(self, path: str, lines: int = 10) -> str:
        """读取文件前N行"""
        content = self._read_file(path)
        if content.startswith("❌"):
            return content
        
        lines_list = content.split('\n')[:lines]
        return '\n'.join(lines_list)
    
    def tail(self, path: str, lines: int = 10) -> str:
        """读取文件后N行"""
        content = self._read_file(path)
        if content.startswith("❌"):
            return content
        
        lines_list = content.split('\n')[-lines:]
        return '\n'.join(lines_list)
    
    def grep(self, pattern: str, path: str) -> str:
        """在文件中搜索"""
        content = self._read_file(path)
        if content.startswith("❌"):
            return content
        
        import re
        try:
            regex = re.compile(pattern, re.IGNORECASE)
            matches = []
            for i, line in enumerate(content.split('\n'), 1):
                if regex.search(line):
                    matches.append(f"{i}: {line}")
            
            if not matches:
                return f"未找到匹配 '{pattern}' 的内容"
            
            return f"在 {path} 中找到 {len(matches)} 处匹配:\n\n" + '\n'.join(matches[:50])
        except re.error as e:
            return f"❌ 正则表达式错误: {e}"


# ==================== 便捷函数 ====================

def create_terminal_tool(workspace: str = ".", timeout: int = 30) -> TerminalTool:
    """创建终端工具"""
    config = TerminalConfig(workspace=workspace, timeout=timeout)
    return TerminalTool(config)
