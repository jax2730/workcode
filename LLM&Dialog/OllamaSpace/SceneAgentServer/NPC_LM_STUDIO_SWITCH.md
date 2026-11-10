# NPC 切换到机房 LM Studio（首版）

## 1) 已完成的代码改造

- `extrator/llm/general_chat.py`
  - 不再硬编码 `ChatOllama(qwen2.5)`。
  - 改为通过 `model_factory` 创建模型。
  - 当 provider=LM Studio 时，走 OpenAI 兼容接口直接对话。

- `extrator/llm/npc_system/views.py`
  - `/npc/*` 入口初始化 LLM 时改为 `model_factory`。

- `extrator/llm/model_factory.py`（新增）
  - 统一模型工厂，支持：
    - `ollama`（默认）
    - `lm_studio`（OpenAI 兼容）

## 2) 服务器环境变量（192.168.5.189）

在 Django 服务启动前设置：

```powershell
$env:NPC_LLM_PROVIDER = "lm_studio"
$env:NPC_LM_STUDIO_URL = "http://192.168.5.23:1234/v1"
$env:NPC_LM_STUDIO_MODEL = "qwen3.5"
$env:NPC_LM_STUDIO_API_KEY = "<机房token>"
$env:NPC_LLM_TEMPERATURE = "0.7"
```

说明：

- `NPC_LLM_PROVIDER=lm_studio` 后，`/api/npc/*` 和 `/npc/*` 都会使用 LM Studio。
- 若未设置 provider，默认仍是 ollama（兼容旧行为）。

## 2.1) 可选：改为使用 `config.py`（推荐常驻配置）

在 `SceneAgentServer/config.py` 中配置以下字段即可：

```python
NPC_LLM_PROVIDER = "lm_studio"
NPC_LM_STUDIO_URL = "http://192.168.5.23:1234/v1"
NPC_LM_STUDIO_MODEL = "qwen3.5"
NPC_LM_STUDIO_API_KEY = "<机房token>"
NPC_LLM_TEMPERATURE = 0.7
```

当前读取优先级：`环境变量 > config.py > 默认值`。
这意味着你可以把 `config.py` 作为默认配置，再用环境变量临时覆盖。

## 3) 启动 Django

```powershell
python manage.py runserver 0.0.0.0:8000
```

## 4) 最小验证（先通再优化）

1. 创建会话

```http
POST /api/npc/connect/
```

1. 发起对话（轻量路径）

```http
POST /api/npc/chat/
Content-Type: application/json

{
  "npc_id": "blacksmith",
  "message": "你好，介绍下你自己",
  "player_id": "player_test"
}
```

1. 或完整 NPC 路径

```http
POST /npc/connect/
POST /npc/chat/
```

## 5) 需要向机房负责人确认的信息

- 模型名是否确认为 `qwen3.5`（你提到“qween3.5”，建议确认准确拼写）。
- `LM Studio` 是否启用了鉴权（token 必填还是可空）。
- 192.168.5.189 到 192.168.5.23:1234 的网络是否互通（防火墙/白名单）。
- `/v1/chat/completions` 是否可用（有些部署路径不同）。

## 6) 常见失败定位

- `401/403`：token 不正确或没带 token。
- `404`：`NPC_LM_STUDIO_URL` 路径不对（通常要带 `/v1`）。
- `连接超时`：机房网络策略问题。
- `模型不存在`：`NPC_LM_STUDIO_MODEL` 与服务端模型名不一致。
