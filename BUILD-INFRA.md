# BUILD-INFRA.md — personal build infrastructure

**What this repo is:** a build factory. It runs the upstream workflow `build-uwp` on an
unmodified copy of [gianlucamazza/xllama](https://github.com/gianlucamazza/xllama) and
produces signed MSIX packages for a personal Xbox Series S in Dev Mode.

**What this repo is not:** the upstream project, a place to keep product changes, or a
place to prepare pull requests. Those belong to the fork
[duranlisandro/xllama_matrix](https://github.com/duranlisandro/xllama_matrix).

## Roles

| Repository | Role |
| --- | --- |
| `duranlisandro/xllama_matrix` (fork) | Keep changes, prepare pull requests to upstream |
| `duranlisandro/xllama-matrix` (this repo, not a fork) | Run GitHub Actions, produce MSIX packages |

GitHub does not register or enable workflows in a fork (its Actions tab offers no way to
enable them: `GET /actions/workflows` returns `total_count: 0` and a dispatch returns HTTP
404, and `PUT /actions/permissions` does not change that). A non-fork copy registers all six
workflows as `active` on first push — hence this repository.

## Branch layout

- `main` — the upstream code as pushed (plus this documentation). Tracks upstream HEAD at
  push time; it carries no product changes.
- `build/head-20261016` — `main` plus **one build-infrastructure change**: the package
  revision is stamped `github.run_number + 1299` in
  `.github/actions/build-uwp-package/action.yml`, so every package version is above the
  installed release and Windows Device Portal accepts an in-place update. Product code is
  untouched.

Why the offset is needed: the workflow stamps the 4th version component with
`github.run_number`, which starts at 1 in a fresh repository. A `1.6.0.1` package is lower
than the installed `1.6.0.1046` and Device Portal refuses the downgrade. Note that GitHub
expressions reject arithmetic inside a `with:` value ("Unexpected symbol: '+'"), which is why
the offset is applied in PowerShell inside the composite action.

## Build a package

```bash
gh workflow run build-uwp.yml --repo duranlisandro/xllama-matrix --ref build/head-20261016
gh run watch <run-id>   --repo duranlisandro/xllama-matrix
gh run download <run-id> --repo duranlisandro/xllama-matrix --name xllama-appx -D /tmp/xllama-ci
```

Artifacts: `xllama-appx` (unified shipping build, ~25 MB) and `xllama-appx-llamacpp`
(bench-only lane). Both include `xllama-test.cer`; MSIX artifacts are retained 14 days.

## Deploy to the console

```bash
source ~/.config/xllama/xbox-env
./scripts/deploy.sh install-cert <xllama-test.cer>      # trust the freshly generated cert
./scripts/deploy.sh <xllama_X.Y.Z.REV_x64.msix>         # in-place update, keeps LocalState
```

Install the MSIX directly. Do **not** use `scripts/install-latest-build.sh`: it performs a
fresh install and wipes LocalState (downloaded models, chat history, settings, and any
`manifest.json` override). Keep the same package identity (`GianlucaMazza.xllama`,
publisher `CN=xllama-dev`) and a higher revision, and models survive the upgrade.

Validation from a host on the same LAN: `scripts/validate-api.sh <spike|chat|budget|tools|framing|embed|prefs|train|all>`.
