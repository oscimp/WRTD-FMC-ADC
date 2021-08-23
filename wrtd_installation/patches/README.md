Most of these patches have been ported and applied to a mainline branch of the concerned repository.
We still need to use all of these patches in our case because of the versionning issue mentionned in the introduction of wrtd_installation`.

So far everything has been ported except for:
- The NVMEM changes (patch 0007, which follows the patch provided by ohwr-build-scripts)
- The DMA mask patch (0018), because it appears to have been already fixed on a more recent version than the one we are using for WRTD.
