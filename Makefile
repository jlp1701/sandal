

.PHONY: kernel

kernel:
	$(MAKE) -C src kernel

runimg:
	$(MAKE) -C src runimg

clean:
	$(MAKE) -C src clean