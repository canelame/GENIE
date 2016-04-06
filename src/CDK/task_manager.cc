#include "CDK/task_manager.h"
#include "CDK/texture.h"
#include "CDK/engine_manager.h"
Task::Task(){
	id_ = 0;
	is_locked_ = false;
}
Task::~Task(){

}
bool Task::getLocked(){
	return is_locked_;
}
void Task::lock(){
	is_locked_ = true;
	}
void Task::unlock(){
	is_locked_ = false;
}
void Task::setId(int id){
  id_ = id;
}
////////////TASK MANAGER///////////////
//////////////////////////////////////
/*std::mutex TaskManager::mutex_;
std::condition_variable TaskManager::cond_variable_;
TaskManager::TaskListT_ TaskManager::task_list_; */
bool stop_;
//bool TaskManager::stop_;
TaskManager* TaskManager::instance_ = NULL;
 TaskManager& TaskManager::instance(){
   if (instance_ == NULL)instance_ = new TaskManager();
   return *instance_;
}

void TaskManager::mainThreadLoop(){

	while (true)
	{
		std::unique_lock<std::mutex> lck(mutex_);

    if (task_list_.empty()){
      cond_variable_.wait(lck);
    }

    if (!task_list_.empty()){
      TaskT_ t;
      t = task_list_.front();
      task_list_.pop_front();
      run_tasks_list_.push_back(t);
          t->runTask();
   }
    else if(stop_){
      break;
    }
    

   }
}

void TaskManager::init(){

	num_cores_ = std::thread::hardware_concurrency()/2;
	for (int i = 0; i < num_cores_;  i++){
		list_thread_.push_back(std::thread(&TaskManager::mainThreadLoop,this));
	}

}
int Task::getId(){
  
  return id_;
}
   void TaskManager::addTask(TaskT_ t){

     std::unique_lock<std::mutex> lock(mutex_);
      t->setId(task_list_.size() + 1);
     task_list_.emplace_back(t);
	   cond_variable_.notify_one();
    

   }

   void TaskManager::waitTask(Task &t){
   
     bool is_finish = false;
    while (!is_finish){
       bool finded = false;
       std::unique_lock<std::mutex> lck(mutex_);
       for (int i = 0; i < run_tasks_list_.size(); i++){
         if (t.getId() == run_tasks_list_[i]->getId()){
           if (run_tasks_list_[i]->finished_){
             is_finish = true;
             run_tasks_list_.erase(run_tasks_list_.begin() + i);
             i = run_tasks_list_.size();//break for

           }
         }
       }
       
       if (is_finish){
         return;
       }
    }
   }

   TaskManager::~TaskManager(){
		 cond_variable_.notify_all();
     mutex_.unlock();
		 stop_ = true;
		 for (int i = 0; i < num_cores_; i++) {
		   list_thread_[i].join();
	   }
	
   }


	 
	 
	 ///////////////TASK IMPLEMENTATIONS//////////////////
	 /////////////////////////////////////////////////////

	 //UPDATE_DISPLAY_LIST_TASK
   UpdateDisplay::UpdateDisplay(DisplayList*dl, Scene *n,bool loaded){
     dl_ = dl;
     nod_ = std::make_shared<Scene>(*n);
     cam_loaded_ = loaded;
     proyex_mat_ = nod_->camera_->getProyection();
     view_mat_ = nod_->camera_->getView();
   
	 }
   void UpdateDisplay::loadLights(){
     
     
     //More lights
     for (int i = 0; i < nod_->lights_.size(); i++){

     }
   }

   void UpdateDisplay::directionalShadowPass(){
     for (std::map<Material*, std::vector<Drawable*>>::iterator it = objects_order_by_program_.begin();
       it != objects_order_by_program_.end(); it++){
       for (int i = 0; i < it->second.size(); i++){
         dl_->add(std::make_shared<SendObjectShadow>(it->second[i]->geometry()->getBuffer().get(),it->second[i]->worldMat()));
       }
     }
     dl_->add(std::make_shared<EndShadowCommand>());
   }

   void UpdateDisplay::runTask(){
     lock();

     //Load lights and create their depth maps
     loadLights();
     //Start rendering the scene into shadow maps
     loadObjects(nod_->root_);
     loadLights();

     dl_->add(std::make_shared<RenderShadowMapCommand>(nod_->directional_light_.get()) );
     directionalShadowPass();

     //dl_->add(std::make_shared<StartShadowCommand>(0, 0));
     for (int i = 0; i < nod_->lights_.size(); i++){
      
     }
    // loadObjects(nod_->root_);

     loadNode(nod_->root_);

     unlock();
   }

   void UpdateDisplay::loadObjects(std::shared_ptr<Node>node){
     std::shared_ptr<Drawable> t_drawable = std::dynamic_pointer_cast<Drawable>(node);
     if (node->getParent() != nullptr){
       node->setWorldMat(node->getParent()->worldMat()*node->modelMat());
     }
     else{
       node->setWorldMat(node->modelMat());
     }
     if (t_drawable){
       Material* material = t_drawable->material().get();
       if (material){
         std::map<Material*, std::vector<Drawable*>>::iterator it;
         it = objects_order_by_program_.find(material);
         if (it != objects_order_by_program_.end()){
           it->second.push_back(t_drawable.get());
         }
         else{
           std::vector < Drawable*>  objs;
           objs.push_back(t_drawable.get());
           objects_order_by_program_.insert(std::pair< Material*, std::vector<Drawable*> >(material, objs));
         }
       }
     }

     for (int i = 0; i < node->size(); i++){
       loadObjects(node->childAt(i));
     }

   }

   void UpdateDisplay::loadNode(std::shared_ptr<Node> node){

    // if (t_drawable){
       for (std::map<Material*, std::vector<Drawable*>>::iterator it = objects_order_by_program_.begin();
         it != objects_order_by_program_.end(); it++){
         //Use program
        
         Material *t_material = it->first;
        
         if (t_material->type_ == 0){  
           dl_->add(std::make_shared<UseMaterialCommand>(t_material));  
         }
         //Render all objects that use this program
         for (int i = 0; i < it->second.size(); i++){
           TextureMaterial::MaterialSettings *mat_sett = (TextureMaterial::MaterialSettings*)it->second[i]->getMaterialSettings().get();
           if (mat_sett->getTextures().size()>0)dl_->add(std::make_shared<UseTextureComman>(t_material->getProgram(), mat_sett->getTextures()));
           Buffer *t_geometry_buff = it->second[i]->geometry()->getBuffer().get();
           if (t_geometry_buff){
             dl_->add(std::make_shared<UseCameraCommand>(proyex_mat_, view_mat_, it->second[i]->worldMat()));
             dl_->add(std::make_shared<LightsCommand>(nod_->lights_, nod_->directional_light_));
             dl_->add(std::make_shared<DrawCommand>(t_geometry_buff));
           }
         }

       }
     finished_ = true;

   }



	 //READ_FILE_TASK
	 ReadFile::ReadFile(const char* file_name,std::string &data){

		 name_ = file_name;
   //  out_file = &data_;
     data = data_;
     
	 };

	 void ReadFile::runTask(){
     lock();
		 std::stringstream temp_vertex_data;
		 std::stringstream temp_fragment_data;
		 std::string line;
		 printf("Reading file task | %s | \n",name_);
		 std::ifstream file_V(name_);
		 if (file_V.is_open()){
			 temp_vertex_data << file_V.rdbuf();
			 file_V.close();
			 line = temp_vertex_data.str();
			 data_ = line;
     
		 }

		 unlock();
     finished_ = true;
	 }

	 //OTHERS TASKS
	 //....
#define STB_IMAGE_IMPLEMENTATION    
#include "external/stb_image.h"

   ReadTexture::ReadTexture(std::shared_ptr<Texture>t, const char*file_name, const char*type){
     name_ = file_name;
     type_ = type;
     texture_ = t;

   }

   void ReadTexture::runTask(){
     lock();
     printf("Texture %s reading\n", name_);
     int comp;
     texture_->loadTexture(name_, type_);
     unlock();
     finished_ = true;
   }


   unsigned int TaskManager::totalTasks(){
     return task_list_.size();
   }
   unsigned int TaskManager::runingTasks(){
     return run_tasks_list_.size();
   }