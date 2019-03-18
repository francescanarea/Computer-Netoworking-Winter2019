/* Name: Francesca Narea
 * Date: 1/9/19
 * Description: Copy one binary file to another.
 */

#include <stdio.h>
#include <string.h>

// argv[1] - src & argv[2] - dst
int main(int argc, char* argv[]){
	// declare binary file
	FILE *src, *dst;			
	char buffer[10]; //how much it will copy at one time 

	src = fopen(argv[1], "rb"); 
	if(src == NULL){  printf("Cannot open %s.\n", argv[1]); }	

	dst = fopen(argv[2], "wb");
	if(dst == NULL){ printf("Cannot open %s.\n", argv[2]); }
	
	int items_read = 1;
	while(items_read != 0){
		items_read = fread(buffer, 1, sizeof(buffer), src);
		fwrite(buffer, items_read, 1, dst);
	}

	fclose(src);
	fclose(dst);
	return(0);
}



