#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <fstream>

#include <iostream>

//std::vector<cv::Point> rampList;
std::vector< std::pair<cv::Point, cv::Point> > rampList;
std::vector<std::vector< cv::Point > > region_list;
std::vector<std::vector< cv::Point > > hull;
std::vector<std::pair<int, int> > region_ramp;

std::vector<cv::Point> region_corners;
int region_label =0;
cv::Mat orig_img;

int click_count =0;
std::vector<cv::Point> rally_point(2);
std::vector<cv::Point> path_way_points;

bool CheckIfExists(cv::Point pt)
{
	for (int i = 0; i < path_way_points.size(); i++)
	{
		cv::Point way_pt = path_way_points[i];
		if (pt.x <= way_pt.x+5 && pt.x >= way_pt.x-5 && pt.y <= way_pt.y+5 && pt.y >= way_pt.y-5 )
		{
			path_way_points.erase(path_way_points.begin() + i);
			return true;
		}
	}
	path_way_points.push_back(pt);
	return false;
}

bool CheckIfExistsRamp(cv::Point pt)
{
	cv::Point  radius(0, 5);
	for (int i = 0; i < rampList.size(); i++)
	{
		cv::Point ralley_1 = rampList[i].second;
		cv::Point ralley_2 = rampList[i].first;
		
		if (pt.x <= ralley_1.x+5 && pt.x >= ralley_1.x-5 && pt.y <= ralley_1.y+5 && pt.y >= ralley_1.y-5 
		|| 	pt.x <= ralley_2.x+5 && pt.x >= ralley_2.x-5 && pt.y <= ralley_2.y+5 && pt.y >= ralley_2.y-5)
		{
			rampList.erase(rampList.begin() + i);
			std::cout<< rampList[i].second << " " << rampList[i].first<< std::endl;
			return true;
		}
	}
	//rampList.push_back(pt);
	return false;
}

void update(int radius, cv::Mat* image, int line_thickness = 2)
{
	for(int i = 0; i < rampList.size(); i++)
	{
		cv::circle(*image, rampList[i].first, radius, cv::Scalar(0,255,0),CV_FILLED  );
		cv::circle(*image, rampList[i].second, radius, cv::Scalar(0,255,0),CV_FILLED  );
	}
	std::vector<std::vector< cv::Point > > hull(region_list.size());
	for(int i = 0; i < path_way_points.size(); i++)
	{
		cv::circle(*image, path_way_points[i], radius, cv::Scalar(0,0,255) );		
	}
	for( int i = 0; i < region_list.size(); i++)
	{  
		cv::convexHull( cv::Mat(region_list[i]), hull[i], false ); 
	}
	for(int i =0; i < region_list.size(); i++)
	{
		cv::drawContours(*image, hull, (int)i, 255, 2);
	}
					
}
void on_mouseCallback(int event, int x, int y, int flags, void *param)
{
	cv::Mat *image = reinterpret_cast<cv::Mat*>(param);
		int radius = 3;
		if(event == cv::EVENT_LBUTTONDBLCLK)
		{
			rally_point[click_count] = cv::Point(x, y);
			click_count++;
			if( CheckIfExistsRamp( cv::Point(x, y) ) )
			{
				orig_img.copyTo(*image);
				update(radius, image);
				click_count=0;
			}
			else if(click_count >= 2)
			{
				cv::circle(*image, rally_point[1], radius, cv::Scalar(0,255,0), CV_FILLED );
				cv::circle(*image, rally_point[0], radius, cv::Scalar(0,255,0), CV_FILLED );
				
				rampList.push_back( std::pair< cv::Point, cv::Point>( rally_point[1], rally_point[0] ) );
				click_count = 0;
			}
		}
		else if(event == cv::EVENT_RBUTTONDBLCLK)
		{
			cv::Point p(x,y);
			if( CheckIfExists( p ) )
			{
				orig_img.copyTo(*image);
				update(radius, image);
			}
			else
			{
				cv::circle(*image, path_way_points.back(), radius, cv::Scalar(0,0,255), CV_FILLED  );
			}
		}
		else if( event ==cv::EVENT_MBUTTONDOWN )
		{
			region_corners.push_back(cv::Point(x,y));
			cv::circle(*image, cv::Point(x,y), radius, cv::Scalar(255,0,0));
		}
		else if( event ==cv::EVENT_MBUTTONDBLCLK )
		{
			region_list.push_back(region_corners);
			region_corners.clear();
			orig_img.copyTo(*image);
			region_label++;
			update(radius, image);
			
		}	
		cv::imshow("Window", *image);
}



double dist(cv::Point p1, cv::Point p2)
{
	return sqrt((p1.x-p2.x)*(p1.x-p2.x)+
		   (p1.y-p2.y)*(p1.y-p2.y));
}


//associates ramps to region definitions by distance
void ramp_to_region()
{
	for(int i = 0; i < rampList.size(); i++)
	{		
		int max = INT_MAX;
		std::pair<int, int> temp(0, i);
		for(int j = 0; j< region_list.size(); j++)
		{
			for(int k = 0; k < region_list[j].size(); k++ )
			{
				if( dist(rampList[i].first, region_list[j][k]) < max)
				{
					temp.first = j;
					max = dist(rampList[i].first, region_list[j][k]);
				}
			}
		}
		region_ramp.push_back(temp);
	}
}


int main()
{
	cv::Mat image, image1;
	image = cv::imread("Images/Savana_river2.jpg");

	image1 = cv::imread("Images/savana_river.jpg");
	int x_ratio = image.cols;
	int y_ratio = image.rows;
	cv::resize(image, image, image1.size());
	image.copyTo(orig_img);
	x_ratio /= image.cols;
	y_ratio /= image.rows;	
	
	std::ofstream output_file("Outputs/assignment.csv",std::ios::out| std::ios::app);
	std::ofstream region_def_file("Outputs/region_definintions.csv",std::ios::out| std::ios::app);
	 
	output_file << "vehicle_name, region_id\n";
	region_def_file << "region_id, region_coordinates, ramp_id, ramp_coordinates\n";
	
	cv::namedWindow("Window");
	cv::setMouseCallback("Window", on_mouseCallback, reinterpret_cast<void*>(&image));
	cv::imshow("Window", image);
	
	while((cv::waitKey() & 0xEFFFFF) != 27);
	
	
	cv::imwrite("Outputs/region_assignments.jpg", image);
	cv::destroyWindow("Window");
	
	ramp_to_region();
	
	std::vector<std::string> vehicle_ids;
	vehicle_ids.push_back("anubis");
	vehicle_ids.push_back("bogey");
	vehicle_ids.push_back("anubis");
	vehicle_ids.push_back("anubis");
	
	//takes region points and orders them into a convex hull
	//puts convex hull point in hull list
	std::vector<std::vector< cv::Point > > hull_list(region_list.size());
	for( int i = 0; i < region_list.size(); i++ )
	{  
		cv::convexHull( cv::Mat(region_list[i]), hull_list[i], false ); 
	}
	
	
	for (int R = 0; R < region_ramp.size(); R++)
	{
		int region_id = region_ramp[R].first;
		int ramp_id   = region_ramp[R].second;
		//assignment of vehicle, region, ramp
		output_file <<vehicle_ids[R]<< ", "<<region_id <<'\n';
		region_def_file <<region_id <<", [";
		for(int j = 0; j < hull_list[region_id].size(); j++)
		{
			if(j == hull_list[region_id].size()-1)
			{
				region_def_file<<y_ratio*hull_list[region_id][j].y << ", " << x_ratio*hull_list[region_id][j].x <<"], ";
			}
			else
			{
				region_def_file << y_ratio*hull_list[region_id][j].y << ", " << x_ratio*hull_list[region_id][j].x <<", " ;
			}
		}
		if(ramp_id >=0)
		{
			region_def_file << ramp_id <<", ";
			region_def_file <<"["<< y_ratio*rampList[ramp_id].first.y << ", " << x_ratio*rampList[ramp_id].first.x <<", "
							<< y_ratio*rampList[ramp_id].second.y << ", " << x_ratio*rampList[ramp_id].second.x <<"]\n";
		}
	}
	
	return 0;
}
