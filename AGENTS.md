# ShooterGame Project Guidance

## Starting Work

Before implementation:

- Read [`docs/arch/architecture.md`](docs/arch/architecture.md) to understand module boundaries.
- Review relevant files under `docs/adr/` for accepted architectural decisions.
- Read relevant files under `docs/systems/` when the task touches a system's contracts, ownership, lifecycle, or data flow.

## Architecture and Validation

- For C++ changes and reviews, follow [code quality](docs/engineering/code-quality.md) and `.editorconfig`. Verify Unreal Engine APIs against the actual `.uproject` engine version and available engine source or official documentation.
- Keep generated binary assets, temporary files, and local planning notes outside Git tracking.
- Do not add test code or smoke-test scripts. Validate changes through necessary Unreal Engine build checks and developer-led in-game testing.
