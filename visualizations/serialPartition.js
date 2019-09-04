
let n = 100;
let A; 
let low, high;
let pivotVal;
let speed = 2000;
let ct = 0;

let swapSteps = 20;
let swapTime = speed;
let swapAmplitude = 1;

let bg = [200,200,200];

function resetAnimation() {
	A = [];
	for(let i = 0; i < n; i++){
		A.push(Math.random());
	}
	low=0; high=n-1;
	pivotVal = Math.random()*0.5+0.25;
	setTimeout(partitionAnimation, speed*1);
}

function setup(){
	let cnv = createCanvas(1000,250);
	cnv.parent("cnv-parent");
	resetAnimation();
}

function drawState() {
	background(bg[0],bg[1],bg[2]);
	noStroke();
	for(let i = 0; i < n; i++){
		if (i == low || i == high){
			stroke(255,255,150); // yellow
			fill(255,0,0); // red
		}
		else if (A[i] < pivotVal) {
			fill(0,0,0); // black
			stroke(255,255,255); // white
		}
		else if (A[i] >= pivotVal){
			fill(255,255,255); // white
			stroke(0,0,0); // black
		}
		rect(i*width/n, height*(1-A[i]), width/n, height*A[i]);
	}
	// pivot value
	noStroke();
	fill(0,255,0, 250); // green
	rect(0,(1-pivotVal)*height,width,5);

	// ct++;
	// save("serialPartition_"+ct+".png");
}

function aniSwap(i, j, ct){
	let xiStart = i*width/n;
	let xjStart = j*width/n;
	let xi = xiStart + ct*(xjStart	- xiStart)/swapSteps;
	let yi = height*(1-A[i]) + swapAmplitude * (ct - 0)* (ct - swapSteps);

	let xj = xjStart - ct*(xjStart	- xiStart)/swapSteps;
	let yj = height*(1-A[j]) + swapAmplitude * (ct - 0)* (ct - swapSteps);

	drawState();

	noStroke();
	fill(bg[0],bg[1],bg[2]);
	rect(i*width/n, 0, width/n, height);
	rect(j*width/n, 0, width/n, height);

	fill(255,0,0);
	stroke(255,255,255);
	rect(xi, yi, width/n, height*A[i]);
	rect(xj, yj, width/n, height*A[j]);


	if(ct < swapSteps){
		setTimeout(function(){ aniSwap(i, j, ct+1); }, swapTime/swapSteps);
	}
	else{
		let tmp = A[high];
		A[high] = A[low];
		A[low] = tmp;
		setTimeout(function(){partitionAnimation();},speed);
	}
}

function partitionAnimation(){
	if(low >= high){
		for(let i = 0; i < 10; i++){
			drawState();
		}
		setTimeout(function(){resetAnimation();}, speed*10)
	}
	else{
		if (A[low] <= pivotVal){
			low++;
		}
		else if(A[high] > pivotVal){
			high--;
		}
		else {
			aniSwap(high, low, 0);
			return;
		}
		drawState();
		setTimeout(partitionAnimation, speed*1);
	}

}

function keyPressed(){
	ct++;
	save("serialPartition_"+ct+".png");
}

