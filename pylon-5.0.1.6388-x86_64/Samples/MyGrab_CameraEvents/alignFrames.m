function alignFrames()
% function alignFrames()
%
% Can we collect multiple frames with short exposure times and then align
% them to minimize blur and maintain dynamic range? How fast can the
% alignment be done?
%
% Could a DNN be designed to do this alignment for a
% fixed number of frames but vehicles going at various speeds? 


%dirRoot = './captures1';
%dirRoot = './captures_mono8';
dirRoot = './captures_cloudy_mono12_1000us';
%dirRoot = '/home/mroos/scratch/';

%d = dir([dirRoot '/*.png']);
d = dir([dirRoot '/*.tiff']);
%d = dir([dirRoot '/*.bmp']);


nPics = length(d);
nPicsPerGroup = 6;
nGroups = nPics/nPicsPerGroup;

[szY szX] = size(imread(fullfile(dirRoot,d(1).name)));

for iGroup = 2:nGroups
   
   fn = 1;
   
   im = nan([szY szX nPicsPerGroup]);
   figure(fn); clf; fn=fn+1;
   for iPic = 1:nPicsPerGroup
      im(:,:,iPic) = double(imread(fullfile(dirRoot,d((iGroup-1)*nPicsPerGroup+iPic).name)));      
   end
   mx = max(im(:));
   for iPic = 1:nPicsPerGroup
      subplot(3,2,iPic);
      imagesc(im(:,:,iPic));
      caxis([0 mx]); colorbar;
   end
  
   % Do cross correlations
   iPic1 = 1;
   iPic2 = 2;
   maxLag = 300;
   
   % Mask options:
   % 1. Multiplying
   % 2. Subtracting
   % 3. Built from two reference images
   %    a. Means of images
   %    b. Differences of images
   %    c. Normalized differences of images
   % 4. Built from all reference images
   %    a. Mean of images
   %    b. Variations of images
   %    c. a and b (e.g., std/mean)

   
   %% Gain mask built from two images
   figure(fn); clf; fn=fn+1;
   
   % Use gain/multiplying mask
   xc_gainMask = zeros(szY,2*maxLag+1);
   %mask = abs(im(:,:,iPic1)-im(:,:,iPic2));
   mask = abs(im(:,:,iPic1)-im(:,:,iPic2))./(im(:,:,iPic1)+im(:,:,iPic2)+eps);
   %mask = sqrt((im(:,:,iPic1)-im(:,:,iPic2)).^2 ./ (im(:,:,iPic1)+im(:,:,iPic2)+eps).^2);
   mask = mask/max(mask(:));
   
%    d = abs(im(:,:,iPic1)-im(:,:,iPic2));
%    th = mean(d(:)) + std(d(:));
%    mask = ones(size(d));
%    mask(d<th) = 0;

   subplot(3,2,1);
   imagesc(mask); colorbar;
   
   cim1 = im(:,:,iPic1).*mask;
   cim2 = im(:,:,iPic2).*mask;
   subplot(3,2,3);
   imagesc(cim1); colorbar;
   subplot(3,2,5);
   imagesc(cim2); colorbar;

   
   ac12 = zeros(szY,2*maxLag+1);
   xc12 = zeros(szY,2*maxLag+1);
   for iY = 1:szY
      fprintf('%d\n',iY);
      xc_gainMask(iY,:) = xcorr(cim1(iY,:),cim2(iY,:),maxLag,'unbiased');
      %xc_gainMask(iY,:) = normDiffCorr(cim1(iY,:),cim2(iY,:),[maxLag maxLag]);
      
      ac12(iY,:) = xcorr((im(iY,:,iPic1)+im(iY,:,iPic2))/2,(im(iY,:,iPic1)+im(iY,:,iPic2))/2,maxLag,'unbiased');
      xc12(iY,:) = xcorr(im(iY,:,iPic1),im(iY,:,iPic2),maxLag,'unbiased');
   end
   subplot(3,2,6)
%    xc = sum(xc_gainMask,1); xc(maxLag+1) = -Inf;
%    plot(-maxLag:maxLag,xc,'.'); gz;
%    title('Gain Mask');   
   xc = sum(xc12)-sum(ac12); xc(maxLag+1) = -Inf;
   plot(-maxLag:maxLag,xc,'.'); gz;
   title('No Mask');
   
   
   [~,ixMax] = max(xc(maxLag+1:end));
   shift1 = ixMax;
   imMean1 = (im(:,:,iPic1) + circshift(im(:,:,iPic2),[0 shift1]))/2;
   subplot(3,2,2);
   imagesc(imMean1); colorbar;
   title(sprintf('shift = %d',shift1));

   
   %% Gain mask built from all images
   figure(fn); clf; fn=fn+1;
   % Use mask generated from all images in group
%    mask = nan([szY szX nPicsPerGroup-1]);
%    for iPic = 1:nPicsPerGroup-1
%       mask(:,:,iPic) = abs(im(:,:,iPic)-im(:,:,iPic+1));
%    end
%    mask = sum(mask,3);
%    mask = mask./bsxfun(@plus,sum(im,3),eps);
%    mask = mask./max(mask(:));
   
%    mn = mean(im,3);
%    vr = var(im,[],3);
   mn = mean(im(:,:,[iPic1 iPic2]),3);
   vr = var(im(:,:,[iPic1 iPic2]),[],3);
   %vr = (im(:,:,iPic1) - im(:,:,iPic2)).^2;
   mask = vr./(mn+eps);
   mask = mask./max(mask(:));
   
   

   for iPic = 1:nPicsPerGroup
      subplot(3,2,iPic);
      imagesc(im(:,:,iPic).*mask); colorbar
   end
   
   figure(fn); clf; fn=fn+1;
 
   subplot(3,2,1);
   imagesc(mask); colorbar;

   cim1 = im(:,:,iPic1).*mask;
   cim2 = im(:,:,iPic2).*mask;
   subplot(3,2,3);
   imagesc(cim1); colorbar;
   subplot(3,2,5);
   imagesc(cim2); colorbar;
   for iY = 1:szY
      fprintf('%d\n',iY);
      xc_gainMask(iY,:) = xcorr(cim1(iY,:),cim2(iY,:),maxLag,'unbiased');
   end
   
   subplot(3,2,6)
   xc = sum(xc_gainMask,1);
   plot(-maxLag:maxLag,xc,'.'); gz;
   title('Gain Mask');

   [~,ixMax] = max(xc(maxLag+1:end));
   shift2 = ixMax;
   imMean2 = (im(:,:,iPic1) + circshift(im(:,:,iPic2),[0 shift2]))/2;
   subplot(3,2,2);
   imagesc(imMean2); colorbar;
   title(sprintf('shift = %d',shift2));
   
   
   %% Subtraction mask built from all images
   mask = mean(im,3);
   figure(fn); clf; fn=fn+1;
   for iPic = 1:nPicsPerGroup
      subplot(3,2,iPic);
      imagesc(im(:,:,iPic)-mask); colorbar
   end
   
   figure(fn); clf; fn=fn+1;
   
   subplot(3,2,1);
   imagesc(mask); colorbar;

   cim1 = (im(:,:,iPic1)-mask);
   cim2 = (im(:,:,iPic2)-mask);
   xc_2d = zeros(szY,2*maxLag+1);
   for iY = 1:szY
      fprintf('%d\n',iY);
      xc_2d(iY,:) = xcorr(cim1(iY,:),cim2(iY,:),maxLag,'unbiased');
   end
   subplot(3,2,3);
   imagesc(cim1); colorbar;
   subplot(3,2,5);
   imagesc(cim2); colorbar;
   
   subplot(3,2,5)
   xc = sum(xc_2d,1);
   plot(-maxLag:maxLag,xc,'.'); gz;
   title('Subtraction Mask');

   [~,ixMax] = max(xc(maxLag+1:end));
   shift3 = ixMax;
   imMean3 = (im(:,:,iPic1) + circshift(im(:,:,iPic2),[0 shift3]))/2;
   subplot(3,2,2);
   imagesc(imMean3); colorbar;
   title(sprintf('shift = %d',shift3));
   
   
   figure(fn); clf; fn=fn+1;
   subplot(3,1,1);
   imagesc(imMean1); colorbar; title(sprintf('shift = %d',shift1));
   subplot(3,1,2);
   imagesc(imMean2); colorbar; title(sprintf('shift = %d',shift2));
   subplot(3,1,3);
   imagesc(imMean3); colorbar; title(sprintf('shift = %d',shift3));
   
   pause
      
end
end


%% SUBFUNCTIONS
function c = normDiffCorr(a,b,maxLags)
   % c = SUM[ (a(x)-b(x-x0)).^2 / (a(x)+b(x-x0)) ]
   % maxLags = [max leftward lag of b; max rightward lag of b]

   % For now, force length of a and b to be same length
   L = length(a);
   if length(b)~=L
      error('a and b must be same length.');
   end
   
   D = bsxfun(@minus,a(:),b(:)').^2;
   S = bsxfun(@plus,a(:),b(:)').^2;

   N = sum(maxLags)+1;
   num = nan(N,1);
   den = nan(N,1);
   for lag = -maxLags(1):maxLags(2)
      num(lag+maxLags(1)+1) = sum(diag(D,lag));
      den(lag+maxLags(1)+1) = sum(diag(S,lag));
   end
   c = flipud(num./den);
end


