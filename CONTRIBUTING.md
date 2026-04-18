# Contributing to hanfts

Thank you for considering a contribution! All contributions are welcome — bug fixes, performance improvements, new language support, or documentation.

## Ideas

Here are some directions, but don't let this list limit you:

- Support for additional languages (Japanese, Chinese, etc.)
- Tokenization improvements
- Performance improvements to the C++ engine
- Bug fixes and edge case handling
- Documentation improvements and translations

## Preparing a pull request

Format, lint, and build before opening a PR:

```bash
./gradlew spotlessApply
./gradlew :hanfts:lintRelease
./gradlew :hanfts:assembleRelease
```

Please correct any failures before requesting a review.

## Code review

All submissions require review, including submissions by project members.
We use GitHub pull requests for this purpose.
