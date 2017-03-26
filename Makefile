target:
	make -C ./sequentiel
	make -C ./Open_MP
	make -C ./Open_MP_MPI
	make -C ./Open_MP_TASK
	make -C ./MPI_Pur
	make -C ./Open_MP_TASK_MPI
	
exec:
	cd Open_MP/ && make exec
	cd sequentiel/ && make exec
	cd Open_MP_MPI/ && make exec
	cd Open_MP_TASK/ && make exec
	cd MPI_Pur/ && make exec
	cd Open_MP_TASK_MPI/ && make exec

clean:
	cd Open_MP/ && make clean
	cd sequentiel/ && make clean
	cd Open_MP_MPI/ && make clean
	cd Open_MP_TASK/ && make clean
	cd MPI_Pur/ && make clean
	cd Open_MP_TASK_MPI/ && make clean
