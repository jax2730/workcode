import argparse
import base64
import json
import mimetypes
import os

import requests

from image_config import (
    CHAT_MODEL,
    LM_STUDIO_API_KEY,
    LM_STUDIO_AUTH_ENV_VAR,
    LM_STUDIO_AUTH_REQUIRED,
    LM_STUDIO_URL,
)


LM_API_KEY_VALUE = None
LM_API_KEY_SOURCE = "none"
LM_API_HEADERS = {}


def resolve_lm_studio_api_key():
    env_name = (LM_STUDIO_AUTH_ENV_VAR or "LM_STUDIO_API_KEY").strip()
    env_value = os.getenv(env_name)
    if env_value and env_value.strip():
        return env_value.strip(), "env"
    cfg_value = (LM_STUDIO_API_KEY or "").strip()
    if cfg_value:
        return cfg_value, "config"
    return None, "none"


def build_lm_headers(api_key):
    if not api_key:
        return {}
    return {"Authorization": f"Bearer {api_key}"}


def configure_lm_auth():
    global LM_API_KEY_VALUE, LM_API_KEY_SOURCE, LM_API_HEADERS
    LM_API_KEY_VALUE, LM_API_KEY_SOURCE = resolve_lm_studio_api_key()
    LM_API_HEADERS = build_lm_headers(LM_API_KEY_VALUE)

    if LM_STUDIO_AUTH_REQUIRED and not LM_API_KEY_VALUE:
        raise RuntimeError(
            f"LM Studio auth is required, but no API key was found. "
            f"Set env var '{LM_STUDIO_AUTH_ENV_VAR}' or LM_STUDIO_API_KEY in config.py."
        )

    if LM_API_KEY_SOURCE == "env":
        print("[LM Studio] auth=enabled (env)")
    elif LM_API_KEY_SOURCE == "config":
        print("[LM Studio] auth=enabled (config)")
    else:
        print("[LM Studio] auth=disabled")


def lm_post(endpoint, payload, timeout=120):
    endpoint = endpoint if endpoint.startswith("/") else f"/{endpoint}"
    safe_headers = dict(LM_API_HEADERS or {})
    if "Authorization" in safe_headers:
        safe_headers["Authorization"] = "<redacted>"
    try:
        print("[LM Studio] POST", f"{LM_STUDIO_URL.rstrip('/')}{endpoint}")
        print("[LM Studio] headers=", json.dumps(safe_headers, ensure_ascii=False))
        print("[LM Studio] payload_preview=", json.dumps(payload, ensure_ascii=False)[:200])
    except Exception:
        pass

    response = requests.post(
        f"{LM_STUDIO_URL.rstrip('/')}{endpoint}",
        json=payload,
        headers=LM_API_HEADERS,
        timeout=timeout,
    )

    if response.status_code in (401, 403):
        print(
            "[LM Studio] Authentication failed. Set LM_STUDIO_API_KEY env var or config fallback, "
            "and confirm LM Studio server token/auth and LM_STUDIO_URL."
        )
    if response.status_code >= 400:
        try:
            err_text = response.text
        except Exception:
            err_text = "<unable to read response body>"
        print(f"[LM Studio] HTTP {response.status_code} on {endpoint}. body={err_text[:1200]}...")
    response.raise_for_status()
    return response


def encode_local_image_as_data_url(image_path):
    if not os.path.exists(image_path):
        raise FileNotFoundError(f"Image file not found: {image_path}")
    mime_type, _ = mimetypes.guess_type(image_path)
    if not mime_type:
        mime_type = "image/png"
    with open(image_path, "rb") as f:
        raw = f.read()
    b64 = base64.b64encode(raw).decode("utf-8")
    return f"data:{mime_type};base64,{b64}"


def query_image(image_path, prompt, model=None, max_tokens=1024, temperature=0.2):
    model_name = str(model or CHAT_MODEL).strip()
    if not model_name:
        raise ValueError("Model is empty. Set CHAT_MODEL in config.py or pass --model.")

    data_url = encode_local_image_as_data_url(image_path)
    payload = {
        "model": model_name,
        "messages": [
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": prompt},
                    {"type": "image_url", "image_url": {"url": data_url}},
                ],
            }
        ],
        "temperature": float(temperature),
        "max_tokens": int(max_tokens),
    }
    response = lm_post("/chat/completions", payload, timeout=300)
    data = response.json()
    choices = data.get("choices") or []
    if not choices:
        return ""
    msg = choices[0].get("message") or {}
    return str(msg.get("content") or "")


def main():
    parser = argparse.ArgumentParser(description="Send image + prompt to LM Studio chat/completions.")
    parser.add_argument("--image", required=True, help="Local image path")
    parser.add_argument("--prompt", required=True, help="Prompt text")
    parser.add_argument("--model", default=None, help="Override model (default: CHAT_MODEL)")
    parser.add_argument("--max-tokens", type=int, default=1024, help="Max completion tokens")
    parser.add_argument("--temperature", type=float, default=0.2, help="Sampling temperature")
    parser.add_argument("--out", default=None, help="Optional output text file path")
    args = parser.parse_args()

    configure_lm_auth()
    result = query_image(
        image_path=args.image,
        prompt=args.prompt,
        model=args.model,
        max_tokens=args.max_tokens,
        temperature=args.temperature,
    )

    if args.out:
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(result)
        print(f"Saved output: {args.out}")
    else:
        print(result)


if __name__ == "__main__":
    main()

