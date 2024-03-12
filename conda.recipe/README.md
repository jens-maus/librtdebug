## Conda build
In order to create conda package:
1. Run: `conda build -c conda-forge conda.recipe`
2. Install locally with `conda install -c conda-forge --use-local librtdebugo` (optional)
3. Upload to conda cloud: `anaconda upload conda-bld/linux-64/librtdebug-<version>.tar.bz2`
